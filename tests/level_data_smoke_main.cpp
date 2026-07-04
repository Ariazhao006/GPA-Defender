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

int tileAt(const LevelDefinition& level, const GridCoord& coord) {
    if (!isValidTile(level, coord)) return -1;
    return level.map[static_cast<std::size_t>(coord.row)][static_cast<std::size_t>(coord.col)];
}

bool isRouteTile(int tile) {
    return tile == 1 || tile == 3 || tile == 4;
}

bool isStraightRouteSegment(const LevelDefinition& level, const GridCoord& from, const GridCoord& to) {
    if (from.row != to.row && from.col != to.col) return false;

    const int rowStep = (to.row > from.row) - (to.row < from.row);
    const int colStep = (to.col > from.col) - (to.col < from.col);

    GridCoord cur = from;
    while (true) {
        if (!isRouteTile(tileAt(level, cur))) return false;
        if (cur.row == to.row && cur.col == to.col) break;
        cur.row += rowStep;
        cur.col += colStep;
    }

    return true;
}

bool pathMatches(const std::vector<GridCoord>& path, const std::vector<GridCoord>& expected) {
    if (path.size() != expected.size()) return false;
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (path[i].row != expected[i].row || path[i].col != expected[i].col) return false;
    }
    return true;
}

std::size_t routeTileCount(const LevelDefinition& level) {
    std::size_t count = 0;
    for (const auto& row : level.map) {
        for (int tile : row) {
            if (isRouteTile(tile)) ++count;
        }
    }
    return count;
}

bool validateLevel(const LevelDefinition& level) {
    if (level.map.empty() || level.map[0].empty()) return false;
    if (level.pathTiles.empty()) return false;
    if (level.waves.empty()) return false;
    if (level.startingGold <= 0) return false;

    for (const auto& path : level.pathTiles) {
        if (path.size() < 2) return false;
        if (tileAt(level, path.front()) != 3) return false;
        if (tileAt(level, path.back()) != 4) return false;
        for (const GridCoord& coord : path) {
            if (!isValidTile(level, coord)) return false;
        }
        for (std::size_t i = 1; i < path.size(); ++i) {
            if (!isStraightRouteSegment(level, path[i - 1], path[i])) return false;
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

bool validateLevel4Layout() {
    const LevelDefinition& level = getLevelDefinition(4);
    if (tileAt(level, {0, 0}) != 3) return false;
    if (tileAt(level, {9, 11}) != 3) return false;
    if (tileAt(level, {5, 6}) != 4) return false;
    if (routeTileCount(level) != 41) return false;
    if (level.pathTiles.size() != 2) return false;
    if (level.pathTiles[0].front().row != 0 || level.pathTiles[0].front().col != 0) return false;
    if (level.pathTiles[1].front().row != 9 || level.pathTiles[1].front().col != 11) return false;
    if (!pathMatches(level.pathTiles[0],
        {{0, 0}, {0, 2}, {4, 2}, {4, 1}, {6, 1}, {6, 3},
         {7, 3}, {7, 4}, {2, 4}, {2, 6}, {5, 6}})) {
        return false;
    }
    if (!pathMatches(level.pathTiles[1],
        {{9, 11}, {6, 11}, {6, 10}, {4, 10}, {4, 8}, {8, 8},
         {8, 6}, {5, 6}})) {
        return false;
    }

    for (const auto& path : level.pathTiles) {
        if (path.back().row != 5 || path.back().col != 6) return false;
    }

    if (level.waves.size() != 3) return false;
    const std::size_t expectedSpawnCounts[] = {7, 10, 11};
    const float expectedLastSpawnTimes[] = {12.0f, 16.2f, 18.0f};
    for (std::size_t i = 0; i < level.waves.size(); ++i) {
        if (i >= 3) return false;
        if (level.waves[i].spawns.size() != expectedSpawnCounts[i]) return false;
        if (level.waves[i].spawns.empty()) return false;
        if (level.waves[i].spawns.back().timeSec < expectedLastSpawnTimes[i]) return false;
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

    if (!validateLevel4Layout()) {
        std::cout << "[Test] Invalid level 4 dual-spawn layout.\n";
        return 1;
    }

    if (GameEngine::towerSpec(TowerKind::Coffee).cost != 80
        || GameEngine::towerSpec(TowerKind::AI).range <= 0.0f
        || GameEngine::towerSpec(TowerKind::Bilibili).cost != 65) {
        std::cout << "[Test] Tower spec contract changed unexpectedly.\n";
        return 1;
    }

    std::cout << "[Test] Level data smoke test passed.\n";
    return 0;
}
