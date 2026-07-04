#pragma once
#ifndef WAVE_MANAGER_H
#define WAVE_MANAGER_H

#include <cstddef>
#include <memory>
#include <vector>

#include "gpa_defender/Enemy.h"
#include "gpa_defender/PlayerStats.h"
#include "gpa_defender/Vector2D.h"

enum class EnemyKind {
    Subject,
    Research,
    Social,
    MorningClass,
    MidtermBoss,
    GroupProject,
    ShortVideo,
    ExamSyllabus,
    PeerPressure
};

struct SpawnEvent {
    float timeSec = 0.0f;
    EnemyKind kind = EnemyKind::Subject;
    int pathId = 0;
};

struct WaveDefinition {
    std::vector<SpawnEvent> spawns;
    int clearBonus = 0;
};

class WaveManager {
private:
    struct ManagedEnemy {
        EnemyKind kind;
        std::unique_ptr<Enemy> enemy;
        bool rewardClaimed = false;
    };

    WaveDefinition activeWave;
    std::vector<std::vector<Vector2D>> paths;
    std::vector<ManagedEnemy> enemies;
    std::size_t nextSpawnIndex = 0;
    float elapsedSec = 0.0f;
    bool running = false;

    std::unique_ptr<Enemy> createEnemy(EnemyKind kind) const;
    const std::vector<Vector2D>& resolvePath(int pathId) const;
    void spawn(const SpawnEvent& event);

public:
    WaveManager();
    explicit WaveManager(std::vector<std::vector<Vector2D>> pathDefinitions);

    void setPaths(std::vector<std::vector<Vector2D>> pathDefinitions);
    void start(const WaveDefinition& wave);
    void reset();

    void updateSpawning(float deltaTime);
    int updateEnemies(float deltaTime);

    std::vector<Enemy*> getLiveEnemies() const;
    std::vector<bool> captureAliveStates() const;
    int collectKillRewards(const std::vector<bool>& aliveBeforeTowerUpdate);

    bool hasPendingSpawns() const;
    bool hasLiveEnemies() const;
    bool isWaveCleared() const;

    int getActiveEnemyCount() const;
    int getSpawnedCount() const;
    int getTotalSpawnCount() const;
    float getElapsedSec() const { return elapsedSec; }

    static int rewardFor(EnemyKind kind);
    static const char* enemyKindName(EnemyKind kind);
    static std::vector<std::vector<Vector2D>> defaultPaths();
};

#endif // WAVE_MANAGER_H

