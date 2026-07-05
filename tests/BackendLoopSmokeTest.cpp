#include "gpa_defender/GameEngine.h"

#include <iostream>

bool runBackendLoopSmokeTest() {
    AstiResult mockAsti;
    mockAsti.thresholdAcademic = 40;
    mockAsti.thresholdPhysical = 40;
    mockAsti.thresholdMental = 40;
    mockAsti.thresholdConnection = 40;
    mockAsti.tags = { "SmokeTest" };

    std::vector<WaveDefinition> waves = {
        {
            {
                { 0.0f, EnemyKind::Subject, 0 }
            },
            10
        }
    };

    std::vector<std::vector<Vector2D>> paths = {
        {
            { 100.0f, 200.0f },
            { 500.0f, 200.0f },
            { 500.0f, 600.0f }
        }
    };

    GameEngine engine(waves, paths, 100);
    engine.initializeFromAsti(mockAsti);

    if (!engine.tryBuyTower(TowerKind::Coffee, { 250.0f, 200.0f })) {
        return false;
    }
    if (!engine.startWave()) {
        return false;
    }

    for (int tick = 0; tick < 120 && engine.getPhase() == GamePhase::WaveRunning; ++tick) {
        engine.update(0.1f);
    }

    const GameSnapshot snapshot = engine.getSnapshot();
    const bool passed = snapshot.phase == GamePhase::Victory
        && snapshot.gold >= 80
        && snapshot.currentAcademic >= snapshot.thresholdAcademic
        && snapshot.currentPhysical >= snapshot.thresholdPhysical
        && snapshot.currentMental >= snapshot.thresholdMental
        && snapshot.currentConnection >= snapshot.thresholdConnection;

    if (!passed) {
        std::cout << "[Test] phase=" << GameEngine::phaseName(snapshot.phase)
                  << " gold=" << snapshot.gold
                  << " baseHp=" << snapshot.baseHp
                  << " enemies=" << snapshot.activeEnemies
                  << " spawned=" << snapshot.spawnedEnemies << "/" << snapshot.totalWaveSpawns
                  << " stats=" << snapshot.currentAcademic << "/" << snapshot.thresholdAcademic
                  << "," << snapshot.currentPhysical << "/" << snapshot.thresholdPhysical
                  << "," << snapshot.currentMental << "/" << snapshot.thresholdMental
                  << "," << snapshot.currentConnection << "/" << snapshot.thresholdConnection
                  << "\n";
    }

    return passed;
}
