#include "gpa_defender/GameEngine.h"

#include <algorithm>
#include <utility>

namespace {

constexpr float kPhysicalDecayIntervalSec = 5.0f;
constexpr float kGpaRecoveryIntervalSec = 5.0f;
constexpr float kGpaRecoveryPerTick = 2.5f;
constexpr float kExerciseTowerCadenceScale = 0.65f;
constexpr int kBaseMaxHp = 100;
constexpr int kBaseDamagePerLeak = 10;
constexpr float kTowerRangeScale = 1.5f;

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
    baseHp = kBaseMaxHp;
    levelIndex = 0;
    waveIndex = -1;
    phase = GamePhase::Build;
    physicalDecayTimer = 0.0f;
    gpaRecoveryTimer = 0.0f;
    gpaRecoveryCarryHp = 0.0f;
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

void GameEngine::update(float deltaTime, const std::vector<Enemy*>& extraTargets) {
    if (deltaTime < 0.0f) return;
    if (phase != GamePhase::WaveRunning) return;

    float scaledDelta = deltaTime * timeScale;

    updateSurvivalTimers(scaledDelta);
    if (!player.isAlive()) {
        phase = GamePhase::GameOver;
        return;
    }

    waveManager.updateSpawning(scaledDelta);
    const int leakedEnemies = waveManager.updateEnemies(scaledDelta);
    if (leakedEnemies > 0) {
        baseHp -= leakedEnemies * kBaseDamagePerLeak;
        if (baseHp < 0) baseHp = 0;
    }
    if (!player.isAlive()) {
        phase = GamePhase::GameOver;
        return;
    }
    if (baseHp <= 0) {
        phase = GamePhase::GameOver;
        return;
    }

    std::vector<Enemy*> liveEnemies = waveManager.getLiveEnemies();
    liveEnemies.insert(liveEnemies.end(), extraTargets.begin(), extraTargets.end());
    std::vector<bool> aliveBeforeTowerUpdate = waveManager.captureAliveStates();
    const float towerDeltaTime = player.getExerciseMode()
        ? scaledDelta * kExerciseTowerCadenceScale
        : scaledDelta;

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
    gpaRecoveryTimer = 0.0f;
    gpaRecoveryCarryHp = 0.0f;
}

void GameEngine::setTimeScale(float scale) {
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 5.0f) scale = 5.0f;
    timeScale = scale;
}

void GameEngine::setPaths(std::vector<std::vector<Vector2D>> pathDefinitions) {
    waveManager.setPaths(std::move(pathDefinitions));
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
    snapshot.baseHp = baseHp;
    snapshot.baseMaxHp = kBaseMaxHp;
    return snapshot;
}

SavedEngineState GameEngine::captureSaveState() const {
    SavedEngineState state;
    state.phase = phase;
    state.waveIndex = waveIndex;

    if (phase == GamePhase::WaveRunning) {
        state.phase = (waveIndex <= 0) ? GamePhase::Build : GamePhase::WaveCleared;
        state.waveIndex = waveIndex - 1;
    } else if (phase == GamePhase::PreGame || phase == GamePhase::GameOver || phase == GamePhase::Victory) {
        state.phase = GamePhase::Build;
        state.waveIndex = -1;
    }

    state.gold = gold;
    state.baseHp = baseHp;
    state.exerciseMode = player.getExerciseMode();
    state.timeScale = timeScale;
    state.currentAcademic = player.getCurrentAcademic();
    state.currentPhysical = player.getCurrentPhysical();
    state.currentMental = player.getCurrentMental();
    state.currentConnection = player.getCurrentConnection();
    state.thresholdAcademic = player.getThresholdAcademic();
    state.thresholdPhysical = player.getThresholdPhysical();
    state.thresholdMental = player.getThresholdMental();
    state.thresholdConnection = player.getThresholdConnection();
    state.astiTags = player.getAstiTags();

    for (const std::unique_ptr<DefenseTower>& tower : towers) {
        if (tower == nullptr) continue;
        SavedTowerState towerState;
        towerState.position = tower->getPosition();
        towerState.cooldown = tower->getCooldownTimer();
        if (dynamic_cast<const CoffeeTower*>(tower.get()) != nullptr) {
            towerState.kind = TowerKind::Coffee;
        } else if (const AITower* ai = dynamic_cast<const AITower*>(tower.get())) {
            towerState.kind = TowerKind::AI;
            towerState.aiLevel = ai->getLevel();
        } else if (dynamic_cast<const LibraryTower*>(tower.get()) != nullptr) {
            towerState.kind = TowerKind::Library;
        } else if (dynamic_cast<const ClassTower*>(tower.get()) != nullptr) {
            towerState.kind = TowerKind::Class;
        } else if (const BilibiliTower* bilibili = dynamic_cast<const BilibiliTower*>(tower.get())) {
            towerState.kind = TowerKind::Bilibili;
            towerState.fireDirection = bilibili->getFireDirection();
        }
        state.towers.push_back(towerState);
    }

    return state;
}

void GameEngine::restoreSaveState(const SavedEngineState& state) {
    gold = std::max(0, state.gold);
    baseHp = std::max(0, std::min(kBaseMaxHp, state.baseHp));
    waveIndex = state.waveIndex;
    if (waveIndex < -1) waveIndex = -1;
    if (waveIndex >= static_cast<int>(waves.size())) {
        waveIndex = static_cast<int>(waves.size()) - 1;
    }

    phase = state.phase;
    if (phase == GamePhase::PreGame || phase == GamePhase::WaveRunning
        || phase == GamePhase::GameOver || phase == GamePhase::Victory) {
        phase = (waveIndex < 0) ? GamePhase::Build : GamePhase::WaveCleared;
    }

    player.restoreForSave(
        state.currentAcademic,
        state.currentPhysical,
        state.currentMental,
        state.currentConnection,
        state.thresholdAcademic,
        state.thresholdPhysical,
        state.thresholdMental,
        state.thresholdConnection,
        state.astiTags,
        state.exerciseMode);

    setTimeScale(state.timeScale);
    physicalDecayTimer = 0.0f;
    gpaRecoveryTimer = 0.0f;
    gpaRecoveryCarryHp = 0.0f;
    waveManager.reset();
    towers.clear();

    for (const SavedTowerState& towerState : state.towers) {
        std::unique_ptr<DefenseTower> tower = createTower(towerState.kind);
        if (AITower* ai = dynamic_cast<AITower*>(tower.get())) {
            ai->restoreLevelForSave(towerState.aiLevel);
        }
        if (BilibiliTower* bilibili = dynamic_cast<BilibiliTower*>(tower.get())) {
            bilibili->setFireDirection(towerState.fireDirection);
        }
        tower->restoreBaseState(towerState.position, towerState.cooldown);
        towers.push_back(std::move(tower));
    }
}

bool GameEngine::canBuildNow() const {
    return phase == GamePhase::Build || phase == GamePhase::WaveRunning
        || phase == GamePhase::WaveCleared;
}

void GameEngine::updateSurvivalTimers(float deltaTime) {
    if (player.getExerciseMode()) {
        gpaRecoveryTimer += deltaTime;
        physicalDecayTimer = 0.0f;

        while (gpaRecoveryTimer >= kGpaRecoveryIntervalSec) {
            gpaRecoveryCarryHp += kGpaRecoveryPerTick;
            int heal = static_cast<int>(gpaRecoveryCarryHp);
            if (heal > 0) {
                baseHp += heal;
                if (baseHp > kBaseMaxHp) baseHp = kBaseMaxHp;
                gpaRecoveryCarryHp -= static_cast<float>(heal);
                if (baseHp >= kBaseMaxHp) {
                    gpaRecoveryCarryHp = 0.0f;
                }
            }
            gpaRecoveryTimer -= kGpaRecoveryIntervalSec;
        }
    }
    else {
        physicalDecayTimer += deltaTime;
        gpaRecoveryTimer = 0.0f;
        gpaRecoveryCarryHp = 0.0f;

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
        return "Pre Game";
    case GamePhase::Build:
        return "Build Phase";
    case GamePhase::WaveRunning:
        return "Wave Running";
    case GamePhase::WaveCleared:
        return "Wave Cleared";
    case GamePhase::GameOver:
        return "Game Over";
    case GamePhase::Victory:
        return "Victory";
    }

    return "Unknown";
}

TowerSpec GameEngine::towerSpec(TowerKind kind) {
    switch (kind) {
    case TowerKind::Coffee:
        return {kind, "Coffee", "Small range, huge burst damage", 50, 92.0f * kTowerRangeScale};
    case TowerKind::AI:
        return {kind, "AI", "360 sweep, hits all in range (upgradeable)", 100, 205.0f * kTowerRangeScale};
    case TowerKind::Library:
        return {kind, "Library", "Slows enemies, no direct damage", 120, 228.0f * kTowerRangeScale};
    case TowerKind::Class:
        return {kind, "Class", "Heavy single hit, long cooldown", 80, 172.0f * kTowerRangeScale};
    case TowerKind::Bilibili:
        return {kind, "Bilibili", "Long-range beam, configurable direction", 65, 305.0f * kTowerRangeScale};
    }

    return {TowerKind::Coffee, "Coffee", "Small range, huge burst damage", 50, 92.0f * kTowerRangeScale};
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

