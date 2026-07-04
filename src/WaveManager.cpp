#include "gpa_defender/WaveManager.h"

#include <algorithm>

namespace {

bool isAlive(const Enemy* enemy) {
    return enemy != nullptr && enemy->getState() != EnemyState::DEAD;
}

} // namespace

WaveManager::WaveManager()
    : WaveManager(defaultPaths()) {}

WaveManager::WaveManager(std::vector<std::vector<Vector2D>> pathDefinitions)
    : paths(std::move(pathDefinitions)) {}

void WaveManager::setPaths(std::vector<std::vector<Vector2D>> pathDefinitions) {
    paths = std::move(pathDefinitions);
}

void WaveManager::start(const WaveDefinition& wave) {
    activeWave = wave;
    std::sort(activeWave.spawns.begin(), activeWave.spawns.end(),
        [](const SpawnEvent& a, const SpawnEvent& b) {
            return a.timeSec < b.timeSec;
        });

    enemies.clear();
    nextSpawnIndex = 0;
    elapsedSec = 0.0f;
    running = true;
}

void WaveManager::reset() {
    activeWave = WaveDefinition{};
    enemies.clear();
    nextSpawnIndex = 0;
    elapsedSec = 0.0f;
    running = false;
}

void WaveManager::updateSpawning(float deltaTime) {
    if (!running) return;
    if (deltaTime < 0.0f) return;

    elapsedSec += deltaTime;
    while (nextSpawnIndex < activeWave.spawns.size()
        && activeWave.spawns[nextSpawnIndex].timeSec <= elapsedSec) {
        spawn(activeWave.spawns[nextSpawnIndex]);
        ++nextSpawnIndex;
    }
}

int WaveManager::updateEnemies(float deltaTime) {
    if (!running) return 0;

    int totalLeakDamage = 0;
    for (ManagedEnemy& managed : enemies) {
        if (managed.enemy != nullptr) {
            const bool hadReachedBase = managed.enemy->hasReachedBase();
            managed.enemy->update(deltaTime, nullptr);
            if (!hadReachedBase && managed.enemy->hasReachedBase()) {
                totalLeakDamage += leakDamageFor(managed.kind);
                managed.rewardClaimed = true;
            }
        }
    }
    return totalLeakDamage;
}

std::vector<Enemy*> WaveManager::getLiveEnemies() const {
    std::vector<Enemy*> out;
    out.reserve(enemies.size());

    for (const ManagedEnemy& managed : enemies) {
        if (isAlive(managed.enemy.get())) {
            out.push_back(managed.enemy.get());
        }
    }

    return out;
}

std::vector<bool> WaveManager::captureAliveStates() const {
    std::vector<bool> states;
    states.reserve(enemies.size());

    for (const ManagedEnemy& managed : enemies) {
        states.push_back(isAlive(managed.enemy.get()));
    }

    return states;
}

int WaveManager::collectKillRewards(const std::vector<bool>& aliveBeforeTowerUpdate) {
    int earned = 0;
    const std::size_t count = std::min(aliveBeforeTowerUpdate.size(), enemies.size());

    for (std::size_t i = 0; i < count; ++i) {
        ManagedEnemy& managed = enemies[i];
        if (managed.rewardClaimed) continue;

        const bool wasAliveForTower = aliveBeforeTowerUpdate[i];
        const bool isDeadNow = managed.enemy == nullptr
            || managed.enemy->getState() == EnemyState::DEAD;

        if (wasAliveForTower && isDeadNow) {
            earned += rewardFor(managed.kind);
            managed.rewardClaimed = true;
        }
    }

    return earned;
}

bool WaveManager::hasPendingSpawns() const {
    return nextSpawnIndex < activeWave.spawns.size();
}

bool WaveManager::hasLiveEnemies() const {
    for (const ManagedEnemy& managed : enemies) {
        if (isAlive(managed.enemy.get())) {
            return true;
        }
    }
    return false;
}

bool WaveManager::isWaveCleared() const {
    return running && !hasPendingSpawns() && !hasLiveEnemies();
}

int WaveManager::getActiveEnemyCount() const {
    int count = 0;
    for (const ManagedEnemy& managed : enemies) {
        if (isAlive(managed.enemy.get())) {
            ++count;
        }
    }
    return count;
}

int WaveManager::getSpawnedCount() const {
    return static_cast<int>(enemies.size());
}

int WaveManager::getTotalSpawnCount() const {
    return static_cast<int>(activeWave.spawns.size());
}

std::unique_ptr<Enemy> WaveManager::createEnemy(EnemyKind kind) const {
    switch (kind) {
    case EnemyKind::Subject:
        return std::make_unique<SubjectEnemy>();
    case EnemyKind::Research:
        return std::make_unique<ResearchEnemy>();
    case EnemyKind::Social:
        return std::make_unique<SocialEnemy>();
    case EnemyKind::MorningClass:
        return std::make_unique<MorningClassEnemy>();
    case EnemyKind::MidtermBoss:
        return std::make_unique<MidtermBossEnemy>();
    case EnemyKind::GroupProject:
        return std::make_unique<GroupProjectEnemy>();
    case EnemyKind::ShortVideo:
        return std::make_unique<ShortVideoEnemy>();
    case EnemyKind::ExamSyllabus:
        return std::make_unique<ExamSyllabusEnemy>();
    case EnemyKind::PeerPressure:
        return std::make_unique<PeerPressureEnemy>();
    }

    return std::make_unique<SubjectEnemy>();
}

const std::vector<Vector2D>& WaveManager::resolvePath(int pathId) const {
    if (pathId >= 0 && static_cast<std::size_t>(pathId) < paths.size()) {
        return paths[static_cast<std::size_t>(pathId)];
    }

    static const std::vector<Vector2D> fallback = {
        { 100.0f, 200.0f },
        { 300.0f, 200.0f },
        { 300.0f, 400.0f }
    };
    return fallback;
}

void WaveManager::spawn(const SpawnEvent& event) {
    std::unique_ptr<Enemy> enemy = createEnemy(event.kind);
    enemy->setPath(resolvePath(event.pathId));
    enemies.push_back(ManagedEnemy{ event.kind, std::move(enemy), false });
}

int WaveManager::rewardFor(EnemyKind kind) {
    switch (kind) {
    case EnemyKind::Subject:
        return 20;
    case EnemyKind::Research:
        return 50;
    case EnemyKind::Social:
        return 15;
    case EnemyKind::MorningClass:
        return 15;
    case EnemyKind::MidtermBoss:
        return 150;
    case EnemyKind::GroupProject:
        return 80;
    case EnemyKind::ShortVideo:
        return 30;
    case EnemyKind::ExamSyllabus:
        return 40;
    case EnemyKind::PeerPressure:
        return 50;
    }

    return 0;
}

int WaveManager::leakDamageFor(EnemyKind kind) {
    switch (kind) {
    case EnemyKind::Subject:
    case EnemyKind::Social:
        return 6;
    case EnemyKind::MorningClass:
    case EnemyKind::ShortVideo:
        return 8;
    case EnemyKind::Research:
    case EnemyKind::GroupProject:
        return 10;
    case EnemyKind::ExamSyllabus:
    case EnemyKind::PeerPressure:
        return 14;
    case EnemyKind::MidtermBoss:
        return 22;
    }

    return 10;
}

const char* WaveManager::enemyKindName(EnemyKind kind) {
    switch (kind) {
    case EnemyKind::Subject:
        return "Subject";
    case EnemyKind::Research:
        return "Research";
    case EnemyKind::Social:
        return "Social";
    case EnemyKind::MorningClass:
        return "MorningClass";
    case EnemyKind::MidtermBoss:
        return "MidtermBoss";
    case EnemyKind::GroupProject:
        return "GroupProject";
    case EnemyKind::ShortVideo:
        return "ShortVideo";
    case EnemyKind::ExamSyllabus:
        return "ExamSyllabus";
    case EnemyKind::PeerPressure:
        return "PeerPressure";
    }

    return "Unknown";
}

std::vector<std::vector<Vector2D>> WaveManager::defaultPaths() {
    return {
        {
            { 100.0f, 200.0f },
            { 300.0f, 200.0f },
            { 300.0f, 400.0f }
        },
        {
            { 80.0f, 120.0f },
            { 240.0f, 120.0f },
            { 240.0f, 360.0f },
            { 420.0f, 360.0f }
        }
    };
}

