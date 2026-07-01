#include "full_flow_scenario.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "gpa_defender/Block.h"
#include "gpa_defender/GameEngine.h"
#include "gpa_defender/Questionnaire.h"

namespace {

void printSnapshot(const std::string& label, const GameSnapshot& snapshot) {
    std::cout << "\n[" << label << "]"
        << " phase=" << GameEngine::phaseName(snapshot.phase)
        << " gold=" << snapshot.gold
        << " wave=" << snapshot.waveIndex
        << " enemies=" << snapshot.activeEnemies << "/" << snapshot.totalWaveSpawns
        << " spawned=" << snapshot.spawnedEnemies
        << " time=" << snapshot.waveTimeSec
        << " exercise=" << (snapshot.exerciseMode ? "on" : "off")
        << "\n  stats A/P/M/C="
        << snapshot.currentAcademic << "/" << snapshot.currentPhysical << "/"
        << snapshot.currentMental << "/" << snapshot.currentConnection
        << " thresholds="
        << snapshot.thresholdAcademic << "/" << snapshot.thresholdPhysical << "/"
        << snapshot.thresholdMental << "/" << snapshot.thresholdConnection
        << std::endl;
}

bool buyTowerAtBlock(GameEngine& engine, Block& blocks, TowerKind kind,
    int row, int col, const Vector2D& direction = { 1.0f, 0.0f }) {
    if (!blocks.canPlaceTower(row, col)) {
        std::cout << "[Test] Cannot place tower at block (" << row << "," << col << ").\n";
        return false;
    }

    const Vector2D position = blocks.getBlockCenter(row, col);
    if (!engine.tryBuyTower(kind, position, direction)) {
        std::cout << "[Test] Not enough gold or wrong phase for tower at block ("
            << row << "," << col << ").\n";
        return false;
    }

    return blocks.placeTowerAt(row, col);
}

bool runCurrentWave(GameEngine& engine, float deltaTime, int maxTicks) {
    for (int tick = 0; tick < maxTicks && engine.getPhase() == GamePhase::WaveRunning; ++tick) {
        engine.update(deltaTime);

        if (tick % 20 == 0) {
            printSnapshot("tick " + std::to_string(tick), engine.getSnapshot());
        }
    }

    return engine.getPhase() == GamePhase::WaveCleared
        || engine.getPhase() == GamePhase::Victory;
}

std::vector<std::vector<Vector2D>> buildTestPaths(const Block& blocks) {
    return {
        {
            blocks.getBlockCenter(3, 0),
            blocks.getBlockCenter(3, 3),
            blocks.getBlockCenter(3, 5),
            blocks.getBlockCenter(5, 5),
            blocks.getBlockCenter(5, 9)
        },
        {
            blocks.getBlockCenter(1, 0),
            blocks.getBlockCenter(1, 4),
            blocks.getBlockCenter(2, 4),
            blocks.getBlockCenter(2, 7),
            blocks.getBlockCenter(4, 7),
            blocks.getBlockCenter(4, 9)
        }
    };
}

} // namespace

int runFullFlowScenario() {
    std::srand(0);

    std::cout << "=== GPA Defenders backend full-flow test ===\n";

    Block blocks(64.0f);
    blocks.buildBlocks({
        {0, 0, 0, 0, 2, 0, 0, 0, 0, 0},
        {3, 1, 1, 1, 1, 1, 1, 1, 1, 4},
        {0, 0, 2, 0, 1, 2, 0, 1, 0, 0},
        {3, 1, 1, 1, 1, 1, 1, 1, 1, 4},
        {0, 0, 0, 0, 0, 2, 0, 2, 0, 0},
        {0, 0, 0, 0, 0, 1, 1, 1, 1, 4},
        {0, 0, 0, 0, 0, 0, 0, 2, 0, 0}
    });

    int testRow = -1;
    int testCol = -1;
    if (!blocks.worldToGrid(96.0f, 96.0f, testRow, testCol)) {
        std::cout << "[Test] Block coordinate conversion failed.\n";
        return 1;
    }
    std::cout << "[Test] world(96,96) -> block(" << testRow << "," << testCol << ")\n";

    if (!blocks.placeObstacleAt(1, 2)) {
        std::cout << "[Test] Obstacle placement failed.\n";
        return 1;
    }
    if (!blocks.removeObstacleAt(1, 2)) {
        std::cout << "[Test] Obstacle removal failed.\n";
        return 1;
    }

    Questionnaire questionnaire = buildAstiQuestionnaire();
    const std::vector<int> answers = { 1, 0, 0, 1, 0, 1, 1, 2, 1 };
    AstiResult asti = questionnaire.score(answers);
    if (asti.tags.empty()) {
        std::cout << "[Test] ASTI scoring failed.\n";
        return 1;
    }

    GameEngine engine(GameEngine::defaultWaves(), buildTestPaths(blocks), 1000);
    engine.initializeFromAsti(asti);
    printSnapshot("after ASTI", engine.getSnapshot());

    if (!buyTowerAtBlock(engine, blocks, TowerKind::AI, 2, 5)) return 1;
    if (!engine.tryUpgradeAiTower(0)) return 1;
    if (!engine.tryUpgradeAiTower(0)) return 1;
    if (!buyTowerAtBlock(engine, blocks, TowerKind::AI, 4, 5)) return 1;
    if (!engine.tryUpgradeAiTower(1)) return 1;
    if (!buyTowerAtBlock(engine, blocks, TowerKind::Library, 2, 2)) return 1;
    if (!buyTowerAtBlock(engine, blocks, TowerKind::Coffee, 4, 7)) return 1;
    if (!buyTowerAtBlock(engine, blocks, TowerKind::Class, 6, 7)) return 1;
    if (!buyTowerAtBlock(engine, blocks, TowerKind::Bilibili, 0, 4, { 1.0f, 0.0f })) return 1;

    printSnapshot("after build", engine.getSnapshot());

    while (engine.getPhase() != GamePhase::Victory && engine.getPhase() != GamePhase::GameOver) {
        if (engine.getPhase() == GamePhase::Build || engine.getPhase() == GamePhase::WaveCleared) {
            const int nextWave = engine.getWaveIndex() + 1;
            engine.setExerciseMode(nextWave == 1);

            std::cout << "\n=== Starting wave " << nextWave << " ===\n";
            if (!engine.startWave()) {
                std::cout << "[Test] Could not start next wave.\n";
                return 1;
            }
        }

        if (!runCurrentWave(engine, 0.25f, 480)) {
            std::cout << "[Test] Wave did not resolve before timeout.\n";
            printSnapshot("timeout", engine.getSnapshot());
            return 1;
        }

        printSnapshot("wave resolved", engine.getSnapshot());
    }

    const GameSnapshot finalSnapshot = engine.getSnapshot();
    printSnapshot("final", finalSnapshot);

    if (finalSnapshot.phase != GamePhase::Victory) {
        std::cout << "[Test] Expected Victory but got "
            << GameEngine::phaseName(finalSnapshot.phase) << ".\n";
        return 1;
    }

    std::cout << "\n=== Full-flow backend test passed ===\n";
    return 0;
}
