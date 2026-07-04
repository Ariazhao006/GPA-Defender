#pragma once

#include <string>

namespace frontend {

struct StageScoreRecord {
    int level = 1;
    int score = 0;
    int gold = 0;
    int baseHp = 0;
    int baseMaxHp = 100;
    int academic = 0;
    int physical = 0;
    int mental = 0;
    int connection = 0;
    int thresholdAcademic = 50;
    int thresholdPhysical = 50;
    int thresholdMental = 50;
    int thresholdConnection = 50;
    int waveIndex = -1;
    std::string timestamp;
};

}  // namespace frontend
