#include "frontend/ChestManager.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace frontend {
namespace {

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

void ChestManager::update(float deltaTime, int currentWave, int totalWaves) {
    (void)currentWave;
    (void)totalWaves;

    if (memoryEffectTimer > 0.0f) {
        memoryEffectTimer -= deltaTime;
        if (memoryEffectTimer < 0.0f) {
            memoryEffectTimer = 0.0f;
        }
    }

    if (effectDisplayTimer > 0.0f) {
        effectDisplayTimer -= deltaTime;
        if (effectDisplayTimer <= 0.0f) {
            clearEffectMessage();
        }
    }

    auto it = chests.begin();
    while (it != chests.end()) {
        if (it->state == ChestState::Active) {
            it->bounceTime += deltaTime * 3.0f;
            it->bounceOffset = std::sin(it->bounceTime) * 5.0f;
            it->timer -= deltaTime;

            if (it->target) {
                it->target->update(deltaTime, nullptr);
            }

            if (it->timer <= 0.0f) {
                it->state = ChestState::Expired;
            } else if (it->target && it->target->getState() == EnemyState::DEAD
                && !it->rewardCollected) {
                it->rewardCollected = true;
                it->state = ChestState::Opened;

                switch (it->type) {
                case ChestType::Memory:
                    memoryEffectTimer = kMemoryEffectDuration;
                    setEffectMessage("Memory Surge: tower damage halved for 10 seconds!");
                    break;
                case ChestType::Hell:
                    hellEventPending = true;
                    setEffectMessage("Hell Mode: an extra boss is incoming!");
                    break;
                case ChestType::Reward:
                    {
                        std::uniform_int_distribution<int> goldDist(100, 200);
                        rewardGold = goldDist(rng);
                        rewardEventPending = true;
                        setEffectMessage("Reward: Gold +" + std::to_string(rewardGold) + "!");
                    }
                    break;
                case ChestType::Gamble:
                    {
                        std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
                        gambleWon = chanceDist(rng) > 0.5f;
                        std::uniform_int_distribution<int> goldDist(50, 150);
                        gambleGold = goldDist(rng);
                        gambleResultPending = true;
                        setEffectMessage(gambleWon
                            ? "Gamble won: Gold +" + std::to_string(gambleGold) + "!"
                            : "Gamble lost: Gold -" + std::to_string(gambleGold) + "!");
                    }
                    break;
                }
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
    if (dist(rng) > 0.6f) {
        return;
    }

    Chest chest;
    chest.position = position;
    chest.type = randomChestType();
    chest.state = ChestState::Active;
    chest.timer = 18.0f;
    chest.target = std::make_unique<TreasureChestEnemy>(
        chestTypeName(chest.type), chestTypeHp(chest.type), 0);
    chest.target->setPosition(position);

    chests.push_back(std::move(chest));
}

void ChestManager::trySpawnChestOnPath(const std::vector<std::vector<Vector2D>>& paths, int currentWave) {
    std::vector<Vector2D> candidates;
    for (const std::vector<Vector2D>& path : paths) {
        if (path.size() <= 2) continue;
        for (std::size_t i = 1; i + 1 < path.size(); ++i) {
            candidates.push_back(path[i]);
        }
    }

    if (candidates.empty()) return;

    std::uniform_int_distribution<int> posDist(0, static_cast<int>(candidates.size()) - 1);
    trySpawnChest(candidates[static_cast<std::size_t>(posDist(rng))], currentWave);
}

bool ChestManager::tryOpenChest(const Vector2D& clickPos, float clickRadius) {
    const float radiusSq = clickRadius * clickRadius;
    for (Chest& chest : chests) {
        if (chest.state != ChestState::Active || chest.armedForAttack) continue;
        const float dx = clickPos.x - chest.position.x;
        const float dy = clickPos.y - chest.position.y;
        if (dx * dx + dy * dy <= radiusSq) {
            chest.armedForAttack = true;
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
    if (memoryEffectTimer > 0.0f) {
        return kMemoryAttackMultiplier;
    }
    return 1.0f;
}

bool ChestManager::hasHellEvent() const {
    return hellEventPending;
}

void ChestManager::clearHellEvent() {
    hellEventPending = false;
}

bool ChestManager::hasRewardEvent() const {
    return rewardEventPending;
}

int ChestManager::getRewardGold() const {
    return rewardGold;
}

void ChestManager::clearRewardEvent() {
    rewardEventPending = false;
    rewardGold = 0;
}

bool ChestManager::hasGambleResult() const {
    return gambleResultPending;
}

bool ChestManager::getGambleWon() const {
    return gambleWon;
}

int ChestManager::getGambleGold() const {
    return gambleGold;
}

void ChestManager::clearGambleResult() {
    gambleResultPending = false;
}

void ChestManager::reset() {
    chests.clear();
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

ChestType ChestManager::randomChestType() {
    std::uniform_int_distribution<int> dist(0, 3);
    switch (dist(rng)) {
    case 0: return ChestType::Memory;
    case 1: return ChestType::Hell;
    case 2: return ChestType::Reward;
    case 3: return ChestType::Gamble;
    default: return ChestType::Reward;
    }
}

void ChestManager::cleanupExpiredChests() {
    chests.erase(
        std::remove_if(chests.begin(), chests.end(),
            [](const Chest& c) { return c.state == ChestState::Expired; }),
        chests.end()
    );
}

} // namespace frontend
