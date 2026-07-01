#include "gpa_defender/GameEngine.h"
#include "gpa_defender/LevelData.h"

#include <iostream>

namespace {

bool isValidTile(const LevelDefinition& level, const GridCoord& coord) {
    return coord.row >= 0
        && coord.col >= 0
        && coord.row < static_cast<int>(level.map.size())
        && !level.map.empty()
        && coord.col < static_cast<int>(level.map[static_cast<std::size_t>(coord.row)].size());
}

bool validateLevel(const LevelDefinition& level) {
    if (level.map.empty() || level.map[0].empty()) return false;
    if (level.pathTiles.empty()) return false;
    if (level.waves.empty()) return false;
    if (level.startingGold <= 0) return false;

    for (const auto& path : level.pathTiles) {
        if (path.size() < 2) return false;
        for (const GridCoord& coord : path) {
            if (!isValidTile(level, coord)) return false;
        }
    }

    for (const WaveDefinition& wave : level.waves) {
        if (wave.spawns.empty()) return false;
        for (const SpawnEvent& spawn : wave.spawns) {
            if (spawn.pathId < 0
                || spawn.pathId >= static_cast<int>(level.pathTiles.size())) {
                return false;
            }
        }
    }

    return true;
}

} // namespace

int main() {
    if (maxLevelCount() != 4) {
        std::cout << "[Test] Expected 4 levels.\n";
        return 1;
    }

    for (int level = 1; level <= maxLevelCount(); ++level) {
        if (!validateLevel(getLevelDefinition(level))) {
            std::cout << "[Test] Invalid level data: " << level << "\n";
            return 1;
        }
    }

    if (GameEngine::towerSpec(TowerKind::Coffee).cost != 50
        || GameEngine::towerSpec(TowerKind::AI).range <= 0.0f
        || GameEngine::towerSpec(TowerKind::Bilibili).cost != 65) {
        std::cout << "[Test] Tower spec contract changed unexpectedly.\n";
        return 1;
    }

    std::cout << "[Test] Level data smoke test passed.\n";
    return 0;
}
