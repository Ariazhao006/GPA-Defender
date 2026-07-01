#pragma once
#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include <cstddef>
#include <memory>
#include <vector>

#include "gpa_defender/DefenseTower.h"
#include "gpa_defender/PlayerStats.h"
#include "gpa_defender/Questionnaire.h"
#include "gpa_defender/Vector2D.h"
#include "gpa_defender/WaveManager.h"

enum class GamePhase {
    PreGame,
    Build,
    WaveRunning,
    WaveCleared,
    GameOver,
    Victory
};

enum class TowerKind {
    Coffee,
    AI,
    Library,
    Class,
    Bilibili
};

struct TowerSpec {
    TowerKind kind = TowerKind::Coffee;
    const char* name = "";
    const char* description = "";
    int cost = 0;
    float range = 0.0f;
};

struct GameSnapshot {
    GamePhase phase = GamePhase::PreGame;
    int gold = 0;
    int levelIndex = 0;
    int waveIndex = -1;
    bool exerciseMode = false;

    int currentAcademic = 0;
    int currentPhysical = 0;
    int currentMental = 0;
    int currentConnection = 0;

    int thresholdAcademic = 0;
    int thresholdPhysical = 0;
    int thresholdMental = 0;
    int thresholdConnection = 0;

    int activeEnemies = 0;
    int spawnedEnemies = 0;
    int totalWaveSpawns = 0;
    float waveTimeSec = 0.0f;
};

class GameEngine {
private:
    PlayerStats player;
    int gold = 100;
    int startingGold = 100;
    int levelIndex = 0;
    int waveIndex = -1;
    GamePhase phase = GamePhase::PreGame;

    float physicalDecayTimer = 0.0f;
    float physicalRecoveryTimer = 0.0f;
    float timeScale = 1.0f;

    std::vector<WaveDefinition> waves;
    WaveManager waveManager;
    std::vector<std::unique_ptr<DefenseTower>> towers;

    bool canBuildNow() const;
    void updatePhysicalStat(float deltaTime);
    std::unique_ptr<DefenseTower> createTower(TowerKind kind) const;

public:
    GameEngine();
    GameEngine(std::vector<WaveDefinition> waveDefinitions,
        std::vector<std::vector<Vector2D>> pathDefinitions,
        int initialGold = 100);

    void initializeFromAsti(const AstiResult& result);
    bool startWave();
    void update(float deltaTime);

    bool trySpend(int cost);
    void addGold(int amount);
    bool tryBuyTower(std::unique_ptr<DefenseTower> tower, const Vector2D& position);
    bool tryBuyTower(TowerKind kind, const Vector2D& position,
        const Vector2D& fireDirection = { 1.0f, 0.0f });
    bool tryUpgradeAiTower(std::size_t towerIndex);
    bool tryRotateBilibiliTower(std::size_t towerIndex, int goldCost = 40);

    void setExerciseMode(bool on);
    bool getExerciseMode() const { return player.getExerciseMode(); }

    void setTimeScale(float scale);
    float getTimeScale() const { return timeScale; }

    void setPaths(std::vector<std::vector<Vector2D>> pathDefinitions);

    GameSnapshot getSnapshot() const;
    GamePhase getPhase() const { return phase; }
    int getGold() const { return gold; }
    int getLevelIndex() const { return levelIndex; }
    int getWaveIndex() const { return waveIndex; }
    const PlayerStats& getPlayerStats() const { return player; }
    const std::vector<std::unique_ptr<DefenseTower>>& getTowers() const { return towers; }
    const WaveManager& getWaveManager() const { return waveManager; }

    static const char* phaseName(GamePhase value);
    static TowerSpec towerSpec(TowerKind kind);
    static std::vector<WaveDefinition> defaultWaves();
};

#endif // GAME_ENGINE_H

