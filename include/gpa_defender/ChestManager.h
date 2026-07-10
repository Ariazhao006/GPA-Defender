#pragma once
#ifndef CHEST_MANAGER_H
#define CHEST_MANAGER_H

#include <memory>
#include <random>
#include <string>
#include <vector>

#include "gpa_defender/Enemy.h"
#include "gpa_defender/Vector2D.h"

enum class ChestType {
    Memory,
    Hell,
    Reward,
    Gamble
};

enum class ChestState {
    Hidden,
    Active,
    Opened,
    Expired
};

enum class ChestEventType {
    Targeted,
    MemoryActivated,
    HellBossSpawned,
    RewardGranted,
    GambleWon,
    GambleLost
};

struct ChestEvent {
    ChestEventType type = ChestEventType::Targeted;
    ChestType chestType = ChestType::Reward;
    int amount = 0;
    int pathId = 0;
};

struct Chest {
    Vector2D position;
    ChestType type = ChestType::Reward;
    ChestState state = ChestState::Hidden;
    float timer = 0.0f;
    bool rewardCollected = false;
    bool armedForAttack = false;
    int pathId = 0;
    std::unique_ptr<TreasureChestEnemy> target;

    // Kept for renderer compatibility; the backend advances the animation state.
    float bounceOffset = 0.0f;
    float bounceTime = 0.0f;
};

class ChestManager {
public:
    ChestManager();
    explicit ChestManager(std::vector<std::vector<Vector2D>> pathDefinitions);

    void setPaths(std::vector<std::vector<Vector2D>> pathDefinitions);
    void setRandomSeed(unsigned int seed) { rng.seed(seed); }
    void update(float deltaTime, int currentWave, int totalWaves);

    void trySpawnChest(const Vector2D& position, int currentWave);
    void trySpawnChestOnPath(const std::vector<std::vector<Vector2D>>& paths, int currentWave);
    void trySpawnChestOnPath(int currentWave);
    void spawnChest(ChestType type, const Vector2D& position, int pathId = 0);

    bool tryOpenChest(const Vector2D& clickPos, float clickRadius);

    const std::vector<Chest>& getActiveChests() const { return chests; }
    std::vector<Enemy*> getLiveTargets() const;

    bool isMemoryEffectActive() const;
    float getAttackMultiplier() const;

    bool hasHellEvent() const;
    void clearHellEvent();
    bool hasRewardEvent() const;
    int getRewardGold() const;
    void clearRewardEvent();
    bool hasGambleResult() const;
    bool getGambleWon() const;
    int getGambleGold() const;
    void clearGambleResult();

    bool hasEffectMessage() const { return effectDisplayTimer > 0.0f; }
    const std::string& getEffectMessage() const { return effectMessage; }
    void clearEffectMessage();

    std::vector<ChestEvent> consumeEvents();
    void reset();

private:
    std::vector<Chest> chests;
    std::vector<std::vector<Vector2D>> paths;
    std::vector<ChestEvent> events;

    bool hellEventPending = false;
    bool rewardEventPending = false;
    int rewardGold = 0;
    bool gambleResultPending = false;
    bool gambleWon = false;
    int gambleGold = 0;
    std::string effectMessage;
    float effectDisplayTimer = 0.0f;
    static constexpr float kEffectDisplayDuration = 3.0f;

    float memoryEffectTimer = 0.0f;
    static constexpr float kMemoryEffectDuration = 10.0f;
    static constexpr float kMemoryAttackMultiplier = 0.5f;

    std::mt19937 rng;

    ChestType randomChestType();
    void setEffectMessage(const std::string& msg);
    void resolveOpenedChest(Chest& chest);
};

#endif // CHEST_MANAGER_H
