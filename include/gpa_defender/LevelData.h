#pragma once
#ifndef LEVEL_DATA_H
#define LEVEL_DATA_H

#include <vector>

#include "gpa_defender/WaveManager.h"

struct GridCoord {
    int row = 0;
    int col = 0;
};

struct LevelDefinition {
    int level = 1;
    int startingGold = 300;
    std::vector<std::vector<int>> map;
    std::vector<std::vector<GridCoord>> pathTiles;
    std::vector<WaveDefinition> waves;
};

const LevelDefinition& getLevelDefinition(int level);
int clampLevelIndex(int level);
int maxLevelCount();

#endif // LEVEL_DATA_H
