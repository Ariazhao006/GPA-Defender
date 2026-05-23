#include "gpa_defender/GameEngine.h"

#include <utility>

namespace {

constexpr float kPhysicalDecayIntervalSec = 5.0f;
constexpr float kPhysicalRecoveryIntervalSec = 3.0f;
constexpr float kExerciseTowerCadenceScale = 0.65f;

} // namespace

GameEngine::GameEngine()
    : GameEngine(defaultWaves(), WaveManager::defaultPaths(), 100) {}

GameEngine::GameEngine(std::vector<WaveDefinition> waveDefinitions,
    std::vector<std::vector<Vector2D>> pathDefinitions,
    int initialGold)
    : gold(initialGold),
    startingGold(initialGold),
    waves(std::move(waveDefinitions)),
    waveManager(std::move(pathDefinitions)) {}

void GameEngine::initializeFromAsti(const AstiResult& result) {
    player.applyAstiResult(result);
    player.setExerciseMode(false);
    gold = startingGold;
    levelIndex = 0;
    waveIndex = -1;
    phase = GamePhase::Build;
    physicalDecayTimer = 0.0f;
    physicalRecoveryTimer = 0.0f;
    towers.clear();
    waveManager.reset();
}

bool GameEngine::startWave() {
    if (phase == GamePhase::PreGame
        || phase == GamePhase::WaveRunning
        || phase == GamePhase::GameOver
        || phase == GamePhase::Victory) {
        return false;
    }

    const int nextWaveIndex = waveIndex + 1;
    if (nextWaveIndex >= static_cast<int>(waves.size())) {
        phase = GamePhase::Victory;
        return true;
    }

    waveIndex = nextWaveIndex;
    waveManager.start(waves[static_cast<std::size_t>(waveIndex)]);
    phase = GamePhase::WaveRunning;
    return true;
}

void GameEngine::update(float deltaTime) {
    if (deltaTime < 0.0f) return;
    if (phase != GamePhase::WaveRunning) return;

    updatePhysicalStat(deltaTime);
    if (!player.isAlive()) {
        phase = GamePhase::GameOver;
        return;
    }

    waveManager.updateSpawning(deltaTime);
    waveManager.updateEnemies(deltaTime, player);
    if (!player.isAlive()) {
        phase = GamePhase::GameOver;
        return;
    }

    std::vector<Enemy*> liveEnemies = waveManager.getLiveEnemies();
    std::vector<bool> aliveBeforeTowerUpdate = waveManager.captureAliveStates();
    const float towerDeltaTime = player.getExerciseMode()
        ? deltaTime * kExerciseTowerCadenceScale
        : deltaTime;

    for (std::unique_ptr<DefenseTower>& tower : towers) {
        if (tower != nullptr) {
            tower->update(towerDeltaTime, liveEnemies);
        }
    }

    addGold(waveManager.collectKillRewards(aliveBeforeTowerUpdate));

    if (!player.isAlive()) {
        phase = GamePhase::GameOver;
        return;
    }

    if (waveManager.isWaveCleared()) {
        if (waveIndex >= 0 && waveIndex < static_cast<int>(waves.size())) {
            addGold(waves[static_cast<std::size_t>(waveIndex)].clearBonus);
        }
        phase = (waveIndex + 1 >= static_cast<int>(waves.size()))
            ? GamePhase::Victory
            : GamePhase::WaveCleared;
    }
}

bool GameEngine::trySpend(int cost) {
    if (cost < 0) return false;
    if (gold < cost) return false;
    gold -= cost;
    return true;
}

void GameEngine::addGold(int amount) {
    if (amount <= 0) return;
    gold += amount;
}

bool GameEngine::tryBuyTower(std::unique_ptr<DefenseTower> tower, const Vector2D& position) {
    if (!canBuildNow()) return false;
    if (tower == nullptr) return false;

    const int cost = tower->getCost();
    if (!trySpend(cost)) return false;

    tower->place(position);
    towers.push_back(std::move(tower));
    return true;
}

bool GameEngine::tryBuyTower(TowerKind kind, const Vector2D& position,
    const Vector2D& fireDirection) {
    std::unique_ptr<DefenseTower> tower = createTower(kind);

    if (kind == TowerKind::Bilibili) {
        BilibiliTower* bilibili = dynamic_cast<BilibiliTower*>(tower.get());
        if (bilibili != nullptr) {
            bilibili->setFireDirection(fireDirection);
        }
    }

    return tryBuyTower(std::move(tower), position);
}

bool GameEngine::tryUpgradeAiTower(std::size_t towerIndex) {
    if (!canBuildNow()) return false;
    if (towerIndex >= towers.size()) return false;

    AITower* aiTower = dynamic_cast<AITower*>(towers[towerIndex].get());
    if (aiTower == nullptr) return false;

    return aiTower->upgrade(gold);
}

bool GameEngine::tryRotateBilibiliTower(std::size_t towerIndex, int goldCost) {
    if (!canBuildNow()) return false;
    if (towerIndex >= towers.size()) return false;

    BilibiliTower* bilibili = dynamic_cast<BilibiliTower*>(towers[towerIndex].get());
    if (bilibili == nullptr) return false;

    return bilibili->purchaseDirectionChange(gold, goldCost);
}

void GameEngine::setExerciseMode(bool on) {
    player.setExerciseMode(on);
    physicalDecayTimer = 0.0f;
    physicalRecoveryTimer = 0.0f;
}

GameSnapshot GameEngine::getSnapshot() const {
    GameSnapshot snapshot;
    snapshot.phase = phase;
    snapshot.gold = gold;
    snapshot.levelIndex = levelIndex;
    snapshot.waveIndex = waveIndex;
    snapshot.exerciseMode = player.getExerciseMode();

    snapshot.currentAcademic = player.getCurrentAcademic();
    snapshot.currentPhysical = player.getCurrentPhysical();
    snapshot.currentMental = player.getCurrentMental();
    snapshot.currentConnection = player.getCurrentConnection();

    snapshot.thresholdAcademic = player.getThresholdAcademic();
    snapshot.thresholdPhysical = player.getThresholdPhysical();
    snapshot.thresholdMental = player.getThresholdMental();
    snapshot.thresholdConnection = player.getThresholdConnection();

    snapshot.activeEnemies = waveManager.getActiveEnemyCount();
    snapshot.spawnedEnemies = waveManager.getSpawnedCount();
    snapshot.totalWaveSpawns = waveManager.getTotalSpawnCount();
    snapshot.waveTimeSec = waveManager.getElapsedSec();
    return snapshot;
}

bool GameEngine::canBuildNow() const {
    return phase == GamePhase::Build || phase == GamePhase::WaveCleared;
}

void GameEngine::updatePhysicalStat(float deltaTime) {
    if (player.getExerciseMode()) {
        physicalRecoveryTimer += deltaTime;
        physicalDecayTimer = 0.0f;

        while (physicalRecoveryTimer >= kPhysicalRecoveryIntervalSec) {
            player.changePhysical(+1);
            physicalRecoveryTimer -= kPhysicalRecoveryIntervalSec;
        }
    }
    else {
        physicalDecayTimer += deltaTime;
        physicalRecoveryTimer = 0.0f;

        while (physicalDecayTimer >= kPhysicalDecayIntervalSec) {
            player.changePhysical(-1);
            physicalDecayTimer -= kPhysicalDecayIntervalSec;
        }
    }
}

std::unique_ptr<DefenseTower> GameEngine::createTower(TowerKind kind) const {
    switch (kind) {
    case TowerKind::Coffee:
        return std::make_unique<CoffeeTower>();
    case TowerKind::AI:
        return std::make_unique<AITower>();
    case TowerKind::Library:
        return std::make_unique<LibraryTower>();
    case TowerKind::Class:
        return std::make_unique<ClassTower>();
    case TowerKind::Bilibili:
        return std::make_unique<BilibiliTower>();
    }

    return std::make_unique<CoffeeTower>();
}

const char* GameEngine::phaseName(GamePhase value) {
    switch (value) {
    case GamePhase::PreGame:
        return "PreGame";
    case GamePhase::Build:
        return "Build";
    case GamePhase::WaveRunning:
        return "WaveRunning";
    case GamePhase::WaveCleared:
        return "WaveCleared";
    case GamePhase::GameOver:
        return "GameOver";
    case GamePhase::Victory:
        return "Victory";
    }

    return "Unknown";
}

std::vector<WaveDefinition> GameEngine::defaultWaves() {
    return {
        {
            {
                { 0.0f, EnemyKind::Subject, 0 },
                { 1.5f, EnemyKind::Social, 0 },
                { 2.5f, EnemyKind::Subject, 0 }
            },
            25
        },
        {
            {
                { 0.0f, EnemyKind::Research, 0 },
                { 1.0f, EnemyKind::Subject, 1 },
                { 3.0f, EnemyKind::GroupProject, 0 },
                { 4.5f, EnemyKind::ShortVideo, 1 }
            },
            50
        },
        {
            {
                { 0.0f, EnemyKind::MidtermBoss, 0 },
                { 1.5f, EnemyKind::PeerPressure, 1 },
                { 3.0f, EnemyKind::ExamSyllabus, 0 },
                { 4.5f, EnemyKind::MorningClass, 1 }
            },
            100
        }
    };
}

