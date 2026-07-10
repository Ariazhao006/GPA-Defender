#include "gpa_defender/ChestManager.h"

#include <cmath>
#include <utility>

namespace {

constexpr float kChestSpawnChance = 0.6f;

const char* chestTypeName(ChestType type) {
    switch (type) {
    case ChestType::Memory: return "Memory Chest";
    case ChestType::Hell: return "Hell Chest";
    case ChestType::Reward: return "Reward Chest";
    case ChestType::Gamble: return "Gamble Chest";
    }
    return "Chest";
}

int chestTypeHp(ChestType type) {
    switch (type) {
    case ChestType::Memory: return 180;
    case ChestType::Hell: return 260;
    case ChestType::Reward: return 140;
    case ChestType::Gamble: return 160;
    }
    return 160;
}

} // namespace

ChestManager::ChestManager()
    : rng(std::random_device{}()) {}

ChestManager::ChestManager(std::vector<std::vector<Vector2D>> pathDefinitions)
    : paths(std::move(pathDefinitions)), rng(std::random_device{}()) {}

void ChestManager::setPaths(std::vector<std::vector<Vector2D>> pathDefinitions) {
    paths = std::move(pathDefinitions);
}

void ChestManager::update(float deltaTime, int currentWave, int totalWaves) {
    (void)currentWave;
    (void)totalWaves;
    if (deltaTime < 0.0f) return;

    if (memoryEffectTimer > 0.0f) {
        memoryEffectTimer -= deltaTime;
        if (memoryEffectTimer < 0.0f) memoryEffectTimer = 0.0f;
    }

    if (effectDisplayTimer > 0.0f) {
        effectDisplayTimer -= deltaTime;
        if (effectDisplayTimer <= 0.0f) clearEffectMessage();
    }

    auto it = chests.begin();
    while (it != chests.end()) {
        if (it->state == ChestState::Active) {
            it->bounceTime += deltaTime * 3.0f;
            it->bounceOffset = std::sin(it->bounceTime) * 5.0f;
            it->timer -= deltaTime;

            if (it->target) it->target->update(deltaTime, nullptr);

            if (it->timer <= 0.0f) {
                it->state = ChestState::Expired;
            } else if (it->target && it->target->getState() == EnemyState::DEAD
                       && !it->rewardCollected) {
                it->rewardCollected = true;
                it->state = ChestState::Opened;
                resolveOpenedChest(*it);
            }
        }

        if (it->state == ChestState::Expired || it->state == ChestState::Opened) {
            it = chests.erase(it);
        } else {
            ++it;
        }
    }
}

void ChestManager::trySpawnChest(const Vector2D& position, int currentWave) {
    (void)currentWave;
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    if (dist(rng) > kChestSpawnChance) return;
    spawnChest(randomChestType(), position, 0);
}

void ChestManager::trySpawnChestOnPath(const std::vector<std::vector<Vector2D>>& pathDefinitions,
                                       int currentWave) {
    struct Candidate {
        Vector2D position;
        int pathId;
    };

    std::vector<Candidate> candidates;
    for (std::size_t pathId = 0; pathId < pathDefinitions.size(); ++pathId) {
        const std::vector<Vector2D>& path = pathDefinitions[pathId];
        if (path.size() <= 2) continue;
        for (std::size_t i = 1; i + 1 < path.size(); ++i) {
            candidates.push_back({path[i], static_cast<int>(pathId)});
        }
    }
    if (candidates.empty()) return;

    std::uniform_int_distribution<int> posDist(0, static_cast<int>(candidates.size()) - 1);
    const Candidate& candidate = candidates[static_cast<std::size_t>(posDist(rng))];

    std::uniform_real_distribution<float> spawnDist(0.0f, 1.0f);
    if (spawnDist(rng) > kChestSpawnChance) return;
    spawnChest(randomChestType(), candidate.position, candidate.pathId);
    (void)currentWave;
}

void ChestManager::trySpawnChestOnPath(int currentWave) {
    trySpawnChestOnPath(paths, currentWave);
}

void ChestManager::spawnChest(ChestType type, const Vector2D& position, int pathId) {
    Chest chest;
    chest.position = position;
    chest.type = type;
    chest.state = ChestState::Active;
    chest.timer = 18.0f;
    chest.pathId = pathId;
    chest.target = std::make_unique<TreasureChestEnemy>(
        chestTypeName(chest.type), chestTypeHp(chest.type), 0);
    chest.target->setPosition(position);
    chests.push_back(std::move(chest));
}

bool ChestManager::tryOpenChest(const Vector2D& clickPos, float clickRadius) {
    const float radiusSq = clickRadius * clickRadius;
    for (Chest& chest : chests) {
        if (chest.state != ChestState::Active || chest.armedForAttack) continue;
        const float dx = clickPos.x - chest.position.x;
        const float dy = clickPos.y - chest.position.y;
        if (dx * dx + dy * dy <= radiusSq) {
            chest.armedForAttack = true;
            events.push_back({ChestEventType::Targeted, chest.type, 0, chest.pathId});
            setEffectMessage("Chest targeted. Towers can attack it now.");
            return true;
        }
    }
    return false;
}

std::vector<Enemy*> ChestManager::getLiveTargets() const {
    std::vector<Enemy*> targets;
    targets.reserve(chests.size());
    for (const Chest& chest : chests) {
        if (chest.state == ChestState::Active && chest.armedForAttack && chest.target
            && chest.target->getState() != EnemyState::DEAD) {
            targets.push_back(chest.target.get());
        }
    }
    return targets;
}

bool ChestManager::isMemoryEffectActive() const {
    return memoryEffectTimer > 0.0f;
}

float ChestManager::getAttackMultiplier() const {
    return isMemoryEffectActive() ? kMemoryAttackMultiplier : 1.0f;
}

bool ChestManager::hasHellEvent() const { return hellEventPending; }
void ChestManager::clearHellEvent() { hellEventPending = false; }
bool ChestManager::hasRewardEvent() const { return rewardEventPending; }
int ChestManager::getRewardGold() const { return rewardGold; }
void ChestManager::clearRewardEvent() { rewardEventPending = false; rewardGold = 0; }
bool ChestManager::hasGambleResult() const { return gambleResultPending; }
bool ChestManager::getGambleWon() const { return gambleWon; }
int ChestManager::getGambleGold() const { return gambleGold; }
void ChestManager::clearGambleResult() { gambleResultPending = false; gambleGold = 0; }

std::vector<ChestEvent> ChestManager::consumeEvents() {
    std::vector<ChestEvent> out = std::move(events);
    events.clear();
    return out;
}

void ChestManager::reset() {
    chests.clear();
    events.clear();
    hellEventPending = false;
    rewardEventPending = false;
    rewardGold = 0;
    gambleResultPending = false;
    gambleWon = false;
    gambleGold = 0;
    memoryEffectTimer = 0.0f;
    clearEffectMessage();
}

void ChestManager::clearEffectMessage() {
    effectDisplayTimer = 0.0f;
    effectMessage.clear();
}

void ChestManager::setEffectMessage(const std::string& msg) {
    effectMessage = msg;
    effectDisplayTimer = kEffectDisplayDuration;
}

void ChestManager::resolveOpenedChest(Chest& chest) {
    switch (chest.type) {
    case ChestType::Memory:
        memoryEffectTimer = kMemoryEffectDuration;
        events.push_back({ChestEventType::MemoryActivated, chest.type, 0, chest.pathId});
        setEffectMessage("Memory Surge: tower damage halved for 10 seconds!");
        break;
    case ChestType::Hell:
        hellEventPending = true;
        events.push_back({ChestEventType::HellBossSpawned, chest.type, 0, chest.pathId});
        setEffectMessage("Hell Mode: an extra boss is incoming!");
        break;
    case ChestType::Reward: {
        std::uniform_int_distribution<int> goldDist(100, 200);
        rewardGold = goldDist(rng);
        rewardEventPending = true;
        events.push_back({ChestEventType::RewardGranted, chest.type, rewardGold, chest.pathId});
        setEffectMessage("Reward: Gold +" + std::to_string(rewardGold) + "!");
        break;
    }
    case ChestType::Gamble: {
        events.push_back({ChestEventType::GamblePrompt, chest.type, 0, chest.pathId});
        break;
    }
    }
}

ChestType ChestManager::randomChestType() {
    std::uniform_int_distribution<int> dist(0, 3);
    return static_cast<ChestType>(dist(rng));
}
