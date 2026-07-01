#include "gpa_defender/GameEngine.h"

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
                { 0.0f, EnemyKind::Social, 0 }
            },
            10
        }
    };

    GameEngine engine(waves, WaveManager::defaultPaths(), 100);
    engine.initializeFromAsti(mockAsti);

    if (!engine.tryBuyTower(TowerKind::Coffee, { 250.0f, 200.0f })) {
        return false;
    }
    if (!engine.startWave()) {
        return false;
    }

    for (int tick = 0; tick < 80 && engine.getPhase() == GamePhase::WaveRunning; ++tick) {
        engine.update(0.1f);
    }

    const GameSnapshot snapshot = engine.getSnapshot();
    return snapshot.phase == GamePhase::Victory
        && snapshot.gold >= 75
        && snapshot.currentAcademic >= snapshot.thresholdAcademic
        && snapshot.currentPhysical >= snapshot.thresholdPhysical
        && snapshot.currentMental >= snapshot.thresholdMental
        && snapshot.currentConnection >= snapshot.thresholdConnection;
}

