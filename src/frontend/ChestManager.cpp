#include "frontend/ChestManager.h"

#include <cmath>
#include <algorithm>

namespace frontend {

ChestManager::ChestManager()
    : rng(std::random_device{}()) {}

void ChestManager::update(float deltaTime, int currentWave, int totalWaves) {
    // 更新回忆模式效果计时器
    if (memoryEffectTimer > 0.0f) {
        memoryEffectTimer -= deltaTime;
        if (memoryEffectTimer < 0.0f) {
            memoryEffectTimer = 0.0f;
        }
    }

    // 效果消息显示 3 秒后自动清除
    if (effectDisplayTimer > 0.0f) {
        effectDisplayTimer -= deltaTime;
        if (effectDisplayTimer <= 0.0f) {
            clearEffectMessage();
        }
    }

    // 更新宝箱动画和计时器，同时清理过期
    auto it = chests.begin();
    while (it != chests.end()) {
        if (it->state == ChestState::Active) {
            it->bounceTime += deltaTime * 3.0f;
            it->bounceOffset = std::sin(it->bounceTime) * 5.0f;
            it->timer -= deltaTime;
            if (it->timer <= 0.0f) {
                it->state = ChestState::Expired;
            }
        }
        if (it->state == ChestState::Expired) {
            it = chests.erase(it);
        } else {
            ++it;
        }
    }
}

void ChestManager::trySpawnChest(const Vector2D& position, int currentWave) {
    // 每波有 60% 概率生成宝箱 (dist 在 [0,1] 中，<=0.6 时生成)
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    if (dist(rng) > 0.6f) {
        return;
    }

    Chest chest;
    chest.position = position;
    chest.type = randomChestType();
    chest.state = ChestState::Active;
    chest.timer = 15.0f; // 15秒内必须开启

    chests.push_back(chest);
}

bool ChestManager::tryOpenChest(const Vector2D& clickPos, float clickRadius) {
    float radiusSq = clickRadius * clickRadius;
    for (Chest& chest : chests) {
        if (chest.state != ChestState::Active) continue;

        float dx = chest.position.x - clickPos.x;
        float dy = chest.position.y - clickPos.y;
        float distSq = dx * dx + dy * dy;

        if (distSq <= radiusSq) {
            chest.state = ChestState::Opened;

            // 触发对应效果并设置消息
            switch (chest.type) {
            case ChestType::Memory:
                // 回忆模式：防御塔攻击力下降 10 秒
                memoryEffectTimer = kMemoryEffectDuration;
                setEffectMessage("回忆涌现：防御塔攻击力减半 10 秒！");
                break;

            case ChestType::Hell:
                // 地狱模式：额外 Boss 来袭
                hellEventPending = true;
                setEffectMessage("地狱模式：额外 Boss 即将来袭！");
                break;

            case ChestType::Reward:
                // 奖励模式：金币 +100~200，指标提升
                {
                    std::uniform_int_distribution<int> goldDist(100, 200);
                    rewardGold = goldDist(rng);
                    rewardEventPending = true;
                    setEffectMessage("获得奖励：金币 +" + std::to_string(rewardGold) + "！");
                }
                break;

            case ChestType::Gamble:
                // 博弈小游戏：50% 概率赢或输
                {
                    std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
                    gambleWon = chanceDist(rng) > 0.5f;
                    std::uniform_int_distribution<int> goldDist(50, 150);
                    gambleGold = goldDist(rng);
                    gambleResultPending = true;
                    if (gambleWon) {
                        setEffectMessage("博弈胜利：金币 +" + std::to_string(gambleGold) + "！");
                    } else {
                        setEffectMessage("博弈失败：金币 -" + std::to_string(gambleGold) + "！");
                    }
                }
                break;
            }

            return true;
        }
    }
    return false;
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
