#include "gpa_defender/ChestManager.h"
#include "gpa_defender/GameEngine.h"
#include "frontend/ChestManager.h"

#include <algorithm>
#include <iostream>
#include <type_traits>
#include <vector>

namespace {

AstiResult testAsti() {
    AstiResult result;
    result.thresholdAcademic = 40;
    result.thresholdPhysical = 40;
    result.thresholdMental = 40;
    result.thresholdConnection = 40;
    result.tags = {"ChestTest"};
    return result;
}

std::vector<std::vector<Vector2D>> testPaths() {
    return {
        {{100.0f, 200.0f}, {500.0f, 200.0f}, {900.0f, 200.0f}},
        {{100.0f, 500.0f}, {500.0f, 500.0f}, {900.0f, 500.0f}}
    };
}

bool destroyChest(GameEngine& engine, ChestType type) {
    for (const Chest& chest : engine.getActiveChests()) {
        if (chest.type != type || !chest.target) continue;
        if (!engine.tryArmChest(chest.position, 1.0f)) return false;
        chest.target->takeDamage(100000);
        return true;
    }
    return false;
}

bool testManagerLifecycle() {
    static_assert(std::is_same<frontend::Chest, Chest>::value,
                  "frontend Chest compatibility alias changed");
    static_assert(std::is_same<frontend::ChestManager, ChestManager>::value,
                  "frontend ChestManager compatibility alias changed");

    ChestManager manager(testPaths());
    manager.setRandomSeed(12345);
    for (int i = 0; i < 100; ++i) manager.trySpawnChestOnPath(0);
    const std::size_t spawned = manager.getActiveChests().size();
    if (spawned == 0 || spawned >= 100) return false;

    manager.update(18.1f, 0, 3);
    if (!manager.getActiveChests().empty()) return false;

    manager.spawnChest(ChestType::Memory, {200.0f, 200.0f}, 0);
    if (!manager.tryOpenChest({200.0f, 200.0f}, 1.0f)) return false;
    std::vector<Enemy*> targets = manager.getLiveTargets();
    if (targets.size() != 1) return false;
    targets[0]->takeDamage(100000);
    manager.update(0.0f, 0, 3);
    if (manager.getAttackMultiplier() != 0.5f) return false;
    manager.update(10.1f, 0, 3);
    return manager.getAttackMultiplier() == 1.0f;
}

bool testRewardAndGamble() {
    const std::vector<WaveDefinition> waves = {{{{100.0f, EnemyKind::Subject, 0}}, 0}};

    GameEngine rewardEngine(waves, testPaths(), 500);
    rewardEngine.initializeFromAsti(testAsti());
    rewardEngine.spawnChest(ChestType::Reward, {300.0f, 300.0f}, 0);
    if (!destroyChest(rewardEngine, ChestType::Reward)) return false;
    rewardEngine.update(0.0f);
    std::vector<ChestEvent> rewardEvents = rewardEngine.consumeChestEvents();
    auto reward = std::find_if(rewardEvents.begin(), rewardEvents.end(), [](const ChestEvent& event) {
        return event.type == ChestEventType::RewardGranted;
    });
    if (reward == rewardEvents.end() || reward->amount < 100 || reward->amount > 200) return false;
    if (rewardEngine.getGold() != 500 + reward->amount) return false;

    GameEngine gambleEngine(waves, testPaths(), 500);
    gambleEngine.initializeFromAsti(testAsti());
    gambleEngine.spawnChest(ChestType::Gamble, {300.0f, 300.0f}, 0);
    if (!destroyChest(gambleEngine, ChestType::Gamble)) return false;
    gambleEngine.update(0.0f);
    std::vector<ChestEvent> gambleEvents = gambleEngine.consumeChestEvents();
    auto gamble = std::find_if(gambleEvents.begin(), gambleEvents.end(), [](const ChestEvent& event) {
        return event.type == ChestEventType::GambleWon || event.type == ChestEventType::GambleLost;
    });
    if (gamble == gambleEvents.end() || gamble->amount < 50 || gamble->amount > 150) return false;
    const int expectedGold = gamble->type == ChestEventType::GambleWon
        ? 500 + gamble->amount
        : 500 - gamble->amount;
    return gambleEngine.getGold() == expectedGold;
}

bool testMemoryDamage() {
    const std::vector<WaveDefinition> waves = {{{{0.0f, EnemyKind::Subject, 0}}, 0}};
    GameEngine engine(waves, testPaths(), 500);
    engine.initializeFromAsti(testAsti());
    if (!engine.tryBuyTower(TowerKind::Coffee, {100.0f, 200.0f})) return false;

    engine.spawnChest(ChestType::Memory, {1000.0f, 1000.0f}, 0);
    if (!destroyChest(engine, ChestType::Memory)) return false;
    engine.update(0.0f);
    engine.consumeChestEvents();

    if (!engine.startWave()) return false;
    engine.update(0.1f);
    std::vector<Enemy*> enemies = engine.getWaveManager().getLiveEnemies();
    if (enemies.size() != 1) return false;
    return enemies[0]->getHp() == 196; // 240 - round(88 * 0.5)
}

bool testHellBoss() {
    const std::vector<WaveDefinition> waves = {{{{100.0f, EnemyKind::Subject, 0}}, 0}};
    const auto paths = testPaths();
    GameEngine engine(waves, paths, 500);
    engine.initializeFromAsti(testAsti());
    if (!engine.startWave()) return false;

    engine.spawnChest(ChestType::Hell, {500.0f, 500.0f}, 1);
    if (!destroyChest(engine, ChestType::Hell)) return false;
    engine.update(0.0f);

    const GameSnapshot snapshot = engine.getSnapshot();
    if (snapshot.phase != GamePhase::WaveRunning || snapshot.activeEnemies != 1
        || snapshot.totalWaveSpawns != 2) {
        return false;
    }

    std::vector<Enemy*> enemies = engine.getWaveManager().getLiveEnemies();
    if (enemies.size() != 1) return false;
    const Rect box = enemies[0]->getBoundingBox();
    const Vector2D center{box.x + box.width / 2.0f, box.y + box.height / 2.0f};
    return center.distanceTo(paths[1][0]) < 0.01f;
}

} // namespace

int main() {
    if (!testManagerLifecycle()) {
        std::cout << "[Test] Chest lifecycle test failed.\n";
        return 1;
    }
    if (!testRewardAndGamble()) {
        std::cout << "[Test] Chest reward/gamble test failed.\n";
        return 1;
    }
    if (!testMemoryDamage()) {
        std::cout << "[Test] Chest memory damage test failed.\n";
        return 1;
    }
    if (!testHellBoss()) {
        std::cout << "[Test] Chest hell boss test failed.\n";
        return 1;
    }

    std::cout << "[Test] Backend chest test passed.\n";
    return 0;
}
