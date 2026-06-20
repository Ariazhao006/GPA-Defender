#include "frontend/GameFrontend.h"
#include "frontend/Renderer.h"

#include "gpa_defender/Block.h"
#include "gpa_defender/DefenseTower.h"
#include "gpa_defender/Enemy.h"
#include "gpa_defender/PlayerStats.h"
#include "gpa_defender/WaveManager.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

namespace frontend {

// 大一地图：单出怪点，简单路径
const std::vector<std::vector<int>> GameFrontend::MAP_DATA = {
    // 0  1  2  3  4  5  6  7  8  9 10 11
    {  3, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2 },  // row 0
    {  2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2 },  // row 1
    {  2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2 },  // row 2
    {  2, 2, 2, 1, 1, 1, 1, 1, 1, 2, 2, 2 },  // row 3
    {  2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2 },  // row 4
    {  2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2 },  // row 5
    {  2, 2, 2, 1, 1, 1, 1, 1, 1, 2, 2, 2 },  // row 6
    {  2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2 },  // row 7
    {  2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 4 },  // row 8
    {  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },  // row 9
};

// 大二地图：双出怪点，路径交叉
const std::vector<std::vector<int>> GameFrontend::MAP_DATA_LEVEL2 = {
    // 0  1  2  3  4  5  6  7  8  9 10 11
    {  3, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2 },  // row 0
    {  2, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2 },  // row 1
    {  2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2 },  // row 2
    {  2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2 },  // row 3
    {  2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2 },  // row 4
    {  2, 3, 1, 1, 1, 1, 2, 2, 2, 1, 2, 2 },  // row 5 - 第二个出怪点
    {  2, 2, 2, 2, 2, 1, 1, 1, 2, 1, 2, 2 },  // row 6
    {  2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2 },  // row 7
    {  2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 4 },  // row 8
    {  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },  // row 9
};

// 大三地图：双路径，S型蜿蜒与直线路线
const std::vector<std::vector<int>> GameFrontend::MAP_DATA_LEVEL3 = {
    // 0  1  2  3  4  5  6  7  8  9 10 11
    {  3, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2 },  // row 0
    {  2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2 },  // row 1
    {  2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 2 },  // row 2
    {  2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2, 2 },  // row 3
    {  2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2 },  // row 4
    {  2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2 },  // row 5
    {  2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2 },  // row 6
    {  2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2 },  // row 7
    {  2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 4 },  // row 8
    {  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },  // row 9
};

// 大四地图：双路径，螺旋型路线
const std::vector<std::vector<int>> GameFrontend::MAP_DATA_LEVEL4 = {
    // 0  1  2  3  4  5  6  7  8  9 10 11
    {  3, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2 },  // row 0
    {  2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2 },  // row 1
    {  2, 2, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2 },  // row 2
    {  2, 2, 2, 2, 2, 1, 1, 1, 2, 2, 2, 2 },  // row 3
    {  2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2 },  // row 4
    {  2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2 },  // row 5 - (5,8)改为1
    {  2, 2, 2, 1, 2, 2, 1, 1, 1, 1, 2, 2 },  // row 6
    {  2, 2, 2, 1, 2, 2, 2, 2, 2, 1, 2, 2 },  // row 7
    {  2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 4 },  // row 8
    {  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },  // row 9
};

const char* GameFrontend::towerName(TowerKind kind) const {
    switch (kind) {
    case TowerKind::Coffee:   return "Coffee";
    case TowerKind::AI:       return "AI";
    case TowerKind::Library:  return "Library";
    case TowerKind::Class:    return "Class";
    case TowerKind::Bilibili: return "Bilibili";
    }
    return "???";
}

int GameFrontend::towerCost(TowerKind kind) const {
    switch (kind) {
    case TowerKind::Coffee:   return 50;
    case TowerKind::AI:       return 100;
    case TowerKind::Library:  return 120;
    case TowerKind::Class:    return 80;
    case TowerKind::Bilibili: return 65;
    }
    return 0;
}

const char* GameFrontend::towerDesc(TowerKind kind) const {
    switch (kind) {
    case TowerKind::Coffee:   return "Small range, huge burst damage";
    case TowerKind::AI:       return "360 sweep, hits all in range (upgradeable)";
    case TowerKind::Library:  return "Slows enemies, no direct damage";
    case TowerKind::Class:    return "Heavy single hit, long cooldown";
    case TowerKind::Bilibili: return "Long-range beam, configurable direction";
    }
    return "";
}

void GameFrontend::initMap() {
    switch (currentLevel) {
    case 1: block.buildBlocks(MAP_DATA); break;
    case 2: block.buildBlocks(MAP_DATA_LEVEL2); break;
    case 3: block.buildBlocks(MAP_DATA_LEVEL3); break;
    case 4: block.buildBlocks(MAP_DATA_LEVEL4); break;
    default: block.buildBlocks(MAP_DATA); break;
    }

    highlandPositions.clear();
    const auto& grid = block.getGrid();
    for (int r = 0; r < block.getRows(); ++r) {
        for (int c = 0; c < block.getCols(); ++c) {
            if (grid[r][c].type == TileType::Highland) {
                highlandPositions.push_back(block.getBlockCenter(r, c));
            }
        }
    }
}

void GameFrontend::initPaths() {
    wavePaths.clear();
    switch (currentLevel) {
    case 1:
        // 大一：单路径
        wavePaths = {{
            block.getBlockCenter(0, 0),   // spawn
            block.getBlockCenter(0, 3),   // turn down
            block.getBlockCenter(3, 3),   // turn right
            block.getBlockCenter(3, 8),   // turn down
            block.getBlockCenter(6, 8),   // turn left
            block.getBlockCenter(6, 3),   // turn down
            block.getBlockCenter(8, 3),   // turn right
            block.getBlockCenter(8, 11),  // base
        }};
        break;
    case 2:
        // 大二：双路径
        wavePaths = {
            {
                block.getBlockCenter(0, 0),   // spawn1
                block.getBlockCenter(0, 2),   // turn down
                block.getBlockCenter(1, 2),   // turn right
                block.getBlockCenter(1, 4),   // turn down
                block.getBlockCenter(3, 4),   // turn right
                block.getBlockCenter(3, 7),   // turn down
                block.getBlockCenter(4, 7),   // turn right
                block.getBlockCenter(4, 9),   // turn down
                block.getBlockCenter(8, 9),   // turn right
                block.getBlockCenter(8, 11),  // base
            },
            {
                block.getBlockCenter(5, 1),   // spawn2
                block.getBlockCenter(5, 5),   // turn right
                block.getBlockCenter(6, 5),   // turn down
                block.getBlockCenter(6, 7),   // turn right
                block.getBlockCenter(7, 7),   // turn down
                block.getBlockCenter(7, 9),   // turn right
                block.getBlockCenter(8, 9),   // turn down
                block.getBlockCenter(8, 11),  // base
            }
        };
        break;
    case 3:
        // 大三：双路径同起点，确保怪物只走Path格子
        wavePaths = {
            {
                block.getBlockCenter(0, 0),   // spawn
                block.getBlockCenter(0, 3),   // right to col 3
                block.getBlockCenter(1, 3),   // down to row 1
                block.getBlockCenter(2, 3),   // down to row 2
                block.getBlockCenter(2, 6),   // right to col 6
                block.getBlockCenter(3, 6),   // down to row 3
                block.getBlockCenter(3, 7),   // right to col 7
                block.getBlockCenter(3, 8),   // right to col 8
                block.getBlockCenter(4, 8),   // down to row 4
                block.getBlockCenter(5, 8),   // down to row 5
                block.getBlockCenter(5, 7),   // left to col 7
                block.getBlockCenter(5, 6),   // left to col 6
                block.getBlockCenter(5, 5),   // left to col 5
                block.getBlockCenter(5, 4),   // left to col 4
                block.getBlockCenter(5, 3),   // left to col 3
                block.getBlockCenter(5, 2),   // left to col 2
                block.getBlockCenter(6, 2),   // down to row 6
                block.getBlockCenter(7, 2),   // down to row 7
                block.getBlockCenter(7, 3),   // right to col 3
                block.getBlockCenter(7, 4),   // right to col 4
                block.getBlockCenter(7, 5),   // right to col 5
                block.getBlockCenter(7, 6),   // right to col 6
                block.getBlockCenter(7, 7),   // right to col 7
                block.getBlockCenter(7, 8),   // right to col 8
                block.getBlockCenter(8, 8),   // down to row 8
                block.getBlockCenter(8, 9),   // right to col 9
                block.getBlockCenter(8, 10),  // right to col 10
                block.getBlockCenter(8, 11),  // base
            },
            {
                block.getBlockCenter(0, 0),   // spawn (同一起点)
                block.getBlockCenter(0, 3),   // right to col 3
                block.getBlockCenter(1, 3),   // down to row 1
                block.getBlockCenter(2, 3),   // down to row 2
                block.getBlockCenter(2, 4),   // right to col 4
                block.getBlockCenter(2, 5),   // right to col 5
                block.getBlockCenter(2, 6),   // right to col 6
                block.getBlockCenter(3, 6),   // down to row 3
                block.getBlockCenter(3, 7),   // right to col 7
                block.getBlockCenter(3, 8),   // right to col 8
                block.getBlockCenter(4, 8),   // down to row 4
                block.getBlockCenter(5, 8),   // down to row 5
                block.getBlockCenter(5, 7),   // left to col 7
                block.getBlockCenter(5, 6),   // left to col 6
                block.getBlockCenter(5, 5),   // left to col 5
                block.getBlockCenter(5, 4),   // left to col 4
                block.getBlockCenter(5, 3),   // left to col 3
                block.getBlockCenter(5, 2),   // left to col 2
                block.getBlockCenter(6, 2),   // down to row 6
                block.getBlockCenter(7, 2),   // down to row 7
                block.getBlockCenter(7, 3),   // right to col 3
                block.getBlockCenter(7, 4),   // right to col 4
                block.getBlockCenter(7, 5),   // right to col 5
                block.getBlockCenter(7, 6),   // right to col 6
                block.getBlockCenter(7, 7),   // right to col 7
                block.getBlockCenter(7, 8),   // right to col 8
                block.getBlockCenter(8, 8),   // down to row 8
                block.getBlockCenter(8, 9),   // right to col 9
                block.getBlockCenter(8, 10),  // right to col 10
                block.getBlockCenter(8, 11),  // base
            }
        };
        break;
    case 4:
        // 大四：双路径同起点，每步只移动一格
        wavePaths = {
            {
                block.getBlockCenter(0, 0),   // spawn
                block.getBlockCenter(0, 1),   // right
                block.getBlockCenter(0, 2),   // right
                block.getBlockCenter(0, 3),   // right
                block.getBlockCenter(1, 3),   // down
                block.getBlockCenter(2, 3),   // down
                block.getBlockCenter(2, 4),   // right
                block.getBlockCenter(2, 5),   // right
                block.getBlockCenter(3, 5),   // down
                block.getBlockCenter(3, 6),   // right
                block.getBlockCenter(3, 7),   // right
                block.getBlockCenter(4, 7),   // down
                block.getBlockCenter(4, 8),   // right
                block.getBlockCenter(4, 9),   // right
                block.getBlockCenter(5, 9),   // down
                block.getBlockCenter(5, 8),   // left
                block.getBlockCenter(5, 7),   // left
                block.getBlockCenter(5, 6),   // left
                block.getBlockCenter(5, 5),   // left
                block.getBlockCenter(5, 4),   // left
                block.getBlockCenter(5, 3),   // left
                block.getBlockCenter(6, 3),   // down
                block.getBlockCenter(7, 3),   // down
                block.getBlockCenter(7, 4),   // right
                block.getBlockCenter(7, 5),   // right
                block.getBlockCenter(7, 6),   // right
                block.getBlockCenter(7, 7),   // right
                block.getBlockCenter(7, 8),   // right
                block.getBlockCenter(7, 9),   // right
                block.getBlockCenter(8, 9),   // down
                block.getBlockCenter(8, 10),  // right
                block.getBlockCenter(8, 11),  // base
            },
            {
                block.getBlockCenter(0, 0),   // spawn (同一起点)
                block.getBlockCenter(0, 1),   // right
                block.getBlockCenter(0, 2),   // right
                block.getBlockCenter(0, 3),   // right
                block.getBlockCenter(1, 3),   // down
                block.getBlockCenter(2, 3),   // down
                block.getBlockCenter(2, 4),   // right
                block.getBlockCenter(2, 5),   // right
                block.getBlockCenter(3, 5),   // down
                block.getBlockCenter(3, 6),   // right
                block.getBlockCenter(3, 7),   // right
                block.getBlockCenter(4, 7),   // down
                block.getBlockCenter(4, 8),   // right
                block.getBlockCenter(4, 9),   // right
                block.getBlockCenter(5, 9),   // down
                block.getBlockCenter(5, 8),   // left
                block.getBlockCenter(5, 7),   // left
                block.getBlockCenter(5, 6),   // left
                block.getBlockCenter(6, 6),   // down
                block.getBlockCenter(6, 7),   // right
                block.getBlockCenter(6, 8),   // right
                block.getBlockCenter(6, 9),   // right
                block.getBlockCenter(7, 9),   // down
                block.getBlockCenter(8, 9),   // down
                block.getBlockCenter(8, 10),  // right
                block.getBlockCenter(8, 11),  // base
            }
        };
        break;
    default:
        wavePaths = {{
            block.getBlockCenter(0, 0),
            block.getBlockCenter(8, 11),
        }};
        break;
    }
}

void GameFrontend::initWaves() {
    initWavesForLevel(currentLevel);
}

void GameFrontend::initWavesForLevel(int level) {
    std::vector<WaveDefinition> waves;

    if (level == 1) {
        // 大一: 基础入门，简单敌人
        WaveDefinition w0;
        w0.spawns = {
            {0.0f, EnemyKind::Subject, 0},
            {3.0f, EnemyKind::Subject, 0},
            {6.0f, EnemyKind::Social, 0},
        };
        w0.clearBonus = 50;

        WaveDefinition w1;
        w1.spawns = {
            {0.0f, EnemyKind::Subject, 0},
            {2.0f, EnemyKind::Subject, 0},
            {5.0f, EnemyKind::Social, 0},
            {8.0f, EnemyKind::MorningClass, 0},
        };
        w1.clearBonus = 80;

        WaveDefinition w2;
        w2.spawns = {
            {0.0f, EnemyKind::Subject, 0},
            {2.0f, EnemyKind::MorningClass, 0},
            {5.0f, EnemyKind::Social, 0},
            {8.0f, EnemyKind::MidtermBoss, 0},
        };
        w2.clearBonus = 150;

        waves = {w0, w1, w2};
    } else if (level == 2) {
        // 大二: 双路径，加入 Research
        WaveDefinition w0;
        w0.spawns = {
            {0.0f, EnemyKind::Subject, 0},
            {2.0f, EnemyKind::MorningClass, 1},
            {5.0f, EnemyKind::Social, 0},
            {8.0f, EnemyKind::Research, 1},
        };
        w0.clearBonus = 60;

        WaveDefinition w1;
        w1.spawns = {
            {0.0f, EnemyKind::Subject, 1},
            {2.0f, EnemyKind::Research, 0},
            {5.0f, EnemyKind::MorningClass, 1},
            {8.0f, EnemyKind::ShortVideo, 0},
        };
        w1.clearBonus = 90;

        WaveDefinition w2;
        w2.spawns = {
            {0.0f, EnemyKind::Research, 0},
            {2.0f, EnemyKind::GroupProject, 1},
            {5.0f, EnemyKind::PeerPressure, 0},
            {8.0f, EnemyKind::MidtermBoss, 1},
        };
        w2.clearBonus = 170;

        waves = {w0, w1, w2};
    } else if (level == 3) {
        // 大三: 双路径，敌人更强
        WaveDefinition w0;
        w0.spawns = {
            {0.0f, EnemyKind::Subject, 0},
            {2.0f, EnemyKind::Research, 1},
            {5.0f, EnemyKind::GroupProject, 0},
            {8.0f, EnemyKind::MorningClass, 1},
        };
        w0.clearBonus = 70;

        WaveDefinition w1;
        w1.spawns = {
            {0.0f, EnemyKind::ExamSyllabus, 1},
            {2.0f, EnemyKind::PeerPressure, 0},
            {5.0f, EnemyKind::Research, 1},
            {8.0f, EnemyKind::Social, 0},
        };
        w1.clearBonus = 100;

        WaveDefinition w2;
        w2.spawns = {
            {0.0f, EnemyKind::GroupProject, 0},
            {2.0f, EnemyKind::ExamSyllabus, 1},
            {5.0f, EnemyKind::PeerPressure, 0},
            {8.0f, EnemyKind::MidtermBoss, 1},
        };
        w2.clearBonus = 190;

        waves = {w0, w1, w2};
    } else {
        // 大四: 双路径，全部敌人类型，数量略增
        WaveDefinition w0;
        w0.spawns = {
            {0.0f, EnemyKind::Subject, 0},
            {1.5f, EnemyKind::Subject, 1},
            {3.0f, EnemyKind::Research, 0},
            {5.0f, EnemyKind::Social, 1},
            {7.0f, EnemyKind::MorningClass, 0},
        };
        w0.clearBonus = 80;

        WaveDefinition w1;
        w1.spawns = {
            {0.0f, EnemyKind::GroupProject, 1},
            {2.0f, EnemyKind::ExamSyllabus, 0},
            {4.0f, EnemyKind::PeerPressure, 1},
            {6.0f, EnemyKind::ShortVideo, 0},
            {8.0f, EnemyKind::Research, 1},
        };
        w1.clearBonus = 110;

        WaveDefinition w2;
        w2.spawns = {
            {0.0f, EnemyKind::MidtermBoss, 0},
            {2.0f, EnemyKind::GroupProject, 1},
            {4.0f, EnemyKind::ExamSyllabus, 0},
            {6.0f, EnemyKind::PeerPressure, 1},
            {8.0f, EnemyKind::MidtermBoss, 0},
        };
        w2.clearBonus = 210;

        waves = {w0, w1, w2};
    }

    int startingGold = 300 + (level - 1) * 150;  // 大一300, 大二450, 大三600, 大四750
    engine = GameEngine(waves, wavePaths, startingGold);
}

void GameFrontend::startLevel(int level) {
    currentLevel = std::clamp(level, 1, 4);
    initMap();
    initPaths();
    initWavesForLevel(currentLevel);
    engine.setPaths(wavePaths);
    engine.initializeFromAsti(astiResult);
    chestManager.reset();
    selectedTowerIndex = -1;
    showExerciseGuide = false;
    hoveredRow = -1;
    hoveredCol = -1;
    gameOverMenuSelection = 0;
    victoryMenuSelection = 0;
    currentScreen = Screen::Game;
    audio.startBGM();
}

void GameFrontend::retryCurrentLevel() {
    startLevel(currentLevel);
}

void GameFrontend::goToNextLevel() {
    if (currentLevel < 4) {
        unlockedLevel = std::max(unlockedLevel, currentLevel + 1);
        startLevel(currentLevel + 1);
    }
}

void GameFrontend::initEngine() {
    engine.initializeFromAsti(astiResult);
}

void GameFrontend::run() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "GPA Defender");
    SetTargetFPS(60);
    audio.init();

    // Build a preload string with every glyph the game will ever draw.
    // Must include both ASCII (32-126) and CJK characters, otherwise
    // LoadFontEx only pre-renders one set and the other maps to junk.
    std::string preloadText;
    preloadText.reserve(2048);
    for (int c = 32; c <= 126; ++c) preloadText += static_cast<char>(c);

    questionnaire = buildAstiQuestionnaire();
    for (const auto& q : questionnaire.getQuestions()) {
        preloadText += q.prompt;
        for (const auto& opt : q.options) {
            preloadText += opt.text;
        }
    }
    preloadText +=
        "微积分线代科研大作业社交早八期中考试大魔王小组作业刷不完的短视频薛定谔的考纲同辈压力"
        "你的ASTI类型结果未识别极限卷王随缘活着健身狂魔社交达人"
        "AttendSchoolTypeIndicator上学人格"
        "最终阈值分数被怪兽攻击至该分数以下则失败学业成绩身体健康心理健康联结感按Enter继续进入游戏"
        "塔说明小范围高爆发单体塔会锁定范围内最近的怪兽适合放在路径拐角补刀"
        "全体扫射升级后伤害射程和攻速提升放下后点击该塔按U升级"
        "不直接造成伤害会让范围内怪兽减速适合配合高伤害塔使用"
        "单次攻击很重出手间隔较长适合守住关键路口"
        "沿一个方向直线攻击放置前按R切换朝向付费转向点击高台格子放置当前塔已选中地图上的塔"
        "运动模式开启后会慢慢恢复身体健康代价是防御塔火力节奏下降适合身体指标接近阈值时救急再次点击Exercise可关闭"
        "选择关卡大一大二大三大四完成关卡后会解锁下一关已解锁未解锁重新测试"
        "回忆涌现防御塔攻击力减半秒地狱模式额外Boss即将来袭获得奖励金币博弈胜利失败";

    int cpCount = 0;
    int* codepoints = LoadCodepoints(preloadText.c_str(), &cpCount);

    Font defaultFont = GetFontDefault();
    uiFont = LoadFontEx("C:\\Windows\\Fonts\\simhei.ttf", 64, codepoints, cpCount);
    if (uiFont.texture.id == defaultFont.texture.id) {
        uiFont = LoadFontEx("C:\\Windows\\Fonts\\msyh.ttc", 64, codepoints, cpCount);
    }
    if (uiFont.texture.id == defaultFont.texture.id) {
        uiFont = LoadFontEx("C:\\Windows\\Fonts\\simsun.ttc", 64, codepoints, cpCount);
    }
    if (uiFont.texture.id == defaultFont.texture.id) {
        uiFont = defaultFont;
    }
    UnloadCodepoints(codepoints);
    GenTextureMipmaps(&uiFont.texture);
    SetTextureFilter(uiFont.texture, TEXTURE_FILTER_TRILINEAR);
    setUiFont(uiFont);

    initMap();
    initPaths();
    answers.resize(questionnaire.getQuestions().size(), -1);

    while (!WindowShouldClose()) {
        audio.update();
        switch (currentScreen) {
        case Screen::MainMenu:
            runMainMenu();
            break;
        case Screen::Questionnaire:
            runQuestionnaire();
            break;
        case Screen::AstiSummary:
            runAstiSummary();
            break;
        case Screen::LevelSelect:
            runLevelSelect();
            break;
        case Screen::Game:
            runGame();
            break;
        case Screen::GameOver:
        case Screen::Victory:
            runGame();  // render game state underneath + overlay
            break;
        }
    }

    UnloadFont(uiFont);
    audio.shutdown();
    CloseWindow();
}

void GameFrontend::runMainMenu() {
    if (IsKeyPressed(KEY_ENTER)) {
        audio.playClick();
        currentQuestion = 0;
        answers.assign(answers.size(), -1);
        questionnaire = buildAstiQuestionnaire();
        currentScreen = Screen::Questionnaire;
        return;
    }

    BeginDrawing();
    ClearBackground(Color{20, 20, 35, 255});
    drawMainMenu();
    EndDrawing();
}

void GameFrontend::runQuestionnaire() {
    const auto& questions = questionnaire.getQuestions();

    for (int key = KEY_ONE; key <= KEY_FOUR; ++key) {
        if (IsKeyPressed(key)) {
            audio.playClick();
            int choice = key - KEY_ONE;
            if (choice < static_cast<int>(questions[currentQuestion].options.size())) {
                answers[currentQuestion] = choice;
                if (currentQuestion < static_cast<int>(questions.size()) - 1) {
                    ++currentQuestion;
                } else {
                    astiResult = questionnaire.score(answers);
                    currentScreen = Screen::AstiSummary;
                    return;
                }
            }
        }
    }

    if (IsKeyPressed(KEY_BACKSPACE) && currentQuestion > 0) {
        --currentQuestion;
    }

    BeginDrawing();
    ClearBackground(Color{20, 20, 35, 255});
    drawQuestionnaire(questionnaire, currentQuestion, answers);
    EndDrawing();
}

void GameFrontend::runAstiSummary() {
    if (IsKeyPressed(KEY_ENTER)) {
        audio.playClick();
        currentLevel = 1;
        unlockedLevel = 1;
        selectedTowerIndex = -1;
        showExerciseGuide = false;
        chestManager.reset();
        currentScreen = Screen::LevelSelect;
        return;
    }

    BeginDrawing();
    ClearBackground(Color{20, 20, 35, 255});
    drawAstiSummary(astiResult);
    EndDrawing();
}

void GameFrontend::runLevelSelect() {
    const int cardW = 190;
    const int cardH = 170;
    const int gap = 35;
    const int totalW = cardW * 4 + gap * 3;
    const int startX = SCREEN_WIDTH / 2 - totalW / 2;
    const int cardY = 230;
    const Rectangle retryRect{
        SCREEN_WIDTH / 2.0f - 140.0f,
        660.0f,
        280.0f,
        48.0f
    };

    Vector2 mouse = GetMousePosition();
    int hoveredLevel = 0;
    for (int i = 0; i < 4; ++i) {
        Rectangle card{
            static_cast<float>(startX + i * (cardW + gap)),
            static_cast<float>(cardY),
            static_cast<float>(cardW),
            static_cast<float>(cardH)
        };
        if (CheckCollisionPointRec(mouse, card)) {
            hoveredLevel = i + 1;
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoveredLevel <= unlockedLevel) {
                audio.playClick();
                startLevel(hoveredLevel);
                return;
            }
        }
    }

    if (CheckCollisionPointRec(mouse, retryRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        audio.playClick();
        questionnaire = buildAstiQuestionnaire();
        answers.assign(questionnaire.getQuestions().size(), -1);
        currentQuestion = 0;
        currentLevel = 1;
        unlockedLevel = 1;
        selectedTowerIndex = -1;
        showExerciseGuide = false;
        engine = GameEngine();
        chestManager.reset();
        currentScreen = Screen::MainMenu;
        return;
    }

    BeginDrawing();
    ClearBackground(Color{20, 20, 35, 255});
    drawLevelSelect(unlockedLevel, hoveredLevel);
    EndDrawing();
}

bool GameFrontend::tryPlaceSelectedTower(int row, int col) {
    if (!block.canPlaceTower(row, col)) return false;

    Vector2D pos = block.getBlockCenter(row, col);
    Vector2D dir = bilibiliDir;

    if (!engine.tryBuyTower(selectedTowerKind, pos, dir)) return false;
    if (!block.placeTowerAt(row, col)) {
        // Rollback isn't straightforward; tower is already in engine.
        // For now just mark the tile.
    }
    audio.playTowerPlace();
    return true;
}

void GameFrontend::handleBuildInput() {
    Vector2 mouse = GetMousePosition();

    // UI panel clicks
    if (mouse.x > UI_PANEL_X) {
        int lx = UI_PANEL_X + 15;
        int w = UI_PANEL_WIDTH - 30;
        int yBase = 0;

        // Compute tower selection button y offset
        // After stats + divider + "Towers" label: roughly y = 290 + 24 per button
        // Need to match drawUI layout
        int ty = 250;  // approximate start, will refine
        // Actually let's compute from drawUI:
        // y starts at 20, phase(30), gold(30), wave(20), divider(22), stats(22+20*4=102), divider(22), "Towers"(24)
        // = 20+30+30+20+12+22+102+12+22+24 ≈ 294

        // Let me just check broad regions
        // Tower buttons from y≈294 to y≈294+5*30=444
        // Exercise: ~458
        // Start Wave: ~496

        TowerKind kinds[] = {TowerKind::Coffee, TowerKind::AI, TowerKind::Library,
                            TowerKind::Class, TowerKind::Bilibili};

        // Y positions must match drawUI layout:
        //   Phase(30) Gold(10) Wave(30) Div(12) StatsTitle(22) Stats(80) Gap(8) Div(12) TowersTitle(24)
        //   = 248 for first tower button
        //   Then 5*30 + 8 + 12 = Exercise at 418, +38 = StartWave at 456
        int yTowerStart = 248;
        int yExModeStart = 418;
        int yStartWaveStart = 456;

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            // Tower selection buttons
            for (int i = 0; i < 5; ++i) {
                Rectangle btn = {static_cast<float>(lx), static_cast<float>(yTowerStart + i * 30),
                                 static_cast<float>(w), 26.0f};
                if (CheckCollisionPointRec(mouse, btn)) {
                    audio.playClick();
                    selectedTowerKind = kinds[i];
                    selectedTowerIndex = -1;
                    showExerciseGuide = false;
                }
            }

            // Exercise mode toggle
            Rectangle exBtn = {static_cast<float>(lx), static_cast<float>(yExModeStart),
                               static_cast<float>(w), 30.0f};
            if (CheckCollisionPointRec(mouse, exBtn)) {
                audio.playClick();
                engine.setExerciseMode(!engine.getExerciseMode());
                showExerciseGuide = true;
            }

            // Start Wave button
            Rectangle swBtn = {static_cast<float>(lx), static_cast<float>(yStartWaveStart),
                               static_cast<float>(w), 36.0f};
            if (CheckCollisionPointRec(mouse, swBtn)) {
                audio.playClick();
                engine.startWave();
            }
        }

        // Click on UI panel = don't interact with map
        return;
    }

    // Map interaction
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        // 先检查是否点击了宝箱
        Vector2D mousePos{mouse.x - MAP_OFFSET_X, mouse.y - MAP_OFFSET_Y};
        if (chestManager.tryOpenChest(mousePos, 30.0f)) {
            audio.playClick();
            return;
        }

        int row, col;
        if (screenToGrid(mouse, row, col)) {
            const auto& towers = engine.getTowers();
            int clickedTower = -1;
            float minDist = 30.0f;
            for (size_t i = 0; i < towers.size(); ++i) {
                Vector2 tp = {MAP_OFFSET_X + towers[i]->getPosition().x,
                              MAP_OFFSET_Y + towers[i]->getPosition().y};
                float dist = sqrtf((mouse.x - tp.x) * (mouse.x - tp.x) +
                                   (mouse.y - tp.y) * (mouse.y - tp.y));
                if (dist < minDist) {
                    clickedTower = static_cast<int>(i);
                    minDist = dist;
                }
            }

            if (clickedTower >= 0) {
                audio.playClick();
                selectedTowerIndex = clickedTower;
                showExerciseGuide = false;
            } else {
                tryPlaceSelectedTower(row, col);
                selectedTowerIndex = -1;
                showExerciseGuide = false;
            }
        }
    } else {
        // Hover tracking
        int row, col;
        if (screenToGrid(mouse, row, col)) {
            hoveredRow = row;
            hoveredCol = col;
        } else {
            hoveredRow = -1;
            hoveredCol = -1;
        }
    }

    // Tower upgrade/rotate
    if (selectedTowerIndex >= 0 &&
        selectedTowerIndex < static_cast<int>(engine.getTowers().size())) {
        if (IsKeyPressed(KEY_U)) {
            audio.playClick();
            engine.tryUpgradeAiTower(static_cast<size_t>(selectedTowerIndex));
        }
        if (IsKeyPressed(KEY_R)) {
            audio.playClick();
            engine.tryRotateBilibiliTower(static_cast<size_t>(selectedTowerIndex));
        }
    }

    // Rotate Bilibili placement direction (only when no tower is selected)
    if (selectedTowerKind == TowerKind::Bilibili && IsKeyPressed(KEY_R) &&
        selectedTowerIndex < 0) {
        bilibiliDirIndex = (bilibiliDirIndex + 1) % 4;
        Vector2D dirs[] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
        bilibiliDir = dirs[bilibiliDirIndex];
    }
}

void GameFrontend::handleWaveRunningInput() {
    // 波次运行期间可以点击宝箱
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse = GetMousePosition();
        // 点击UI面板时不处理地图交互
        if (mouse.x > UI_PANEL_X) return;

        Vector2D mousePos{mouse.x - MAP_OFFSET_X, mouse.y - MAP_OFFSET_Y};
        if (chestManager.tryOpenChest(mousePos, 30.0f)) {
            audio.playClick();
        }
    }
}

void GameFrontend::handleGlobalInput() {
    if (IsKeyPressed(KEY_SPACE)) {
        GamePhase phase = engine.getPhase();
        if (phase == GamePhase::Build || phase == GamePhase::WaveCleared) {
            engine.startWave();
        }
    }
    if (IsKeyPressed(KEY_E)) {
        engine.setExerciseMode(!engine.getExerciseMode());
        showExerciseGuide = true;
    }
    if (IsKeyPressed(KEY_EQUAL)) {
        float newScale = engine.getTimeScale() + 0.5f;
        engine.setTimeScale(newScale);
    }
    if (IsKeyPressed(KEY_MINUS)) {
        float newScale = engine.getTimeScale() - 0.5f;
        engine.setTimeScale(newScale);
    }
    if (IsKeyPressed(KEY_ZERO)) {
        engine.setTimeScale(1.0f);
    }
}

void GameFrontend::updateGame(float dt) {
    int aliveBefore = engine.getWaveManager().getActiveEnemyCount();
    GamePhase phaseBefore = engine.getPhase();

    engine.update(dt);

    int aliveAfter = engine.getWaveManager().getActiveEnemyCount();
    GamePhase phaseAfter = engine.getPhase();

    // Enemy death SFX
    int deaths = aliveBefore - aliveAfter;
    for (int i = 0; i < deaths && i < 5; ++i) {
        audio.playEnemyDeath();
    }

    // Phase transition SFX
    if (phaseBefore != phaseAfter) {
        if (phaseAfter == GamePhase::WaveRunning) {
            audio.playWaveStart();
        } else if (phaseAfter == GamePhase::WaveCleared) {
            // 波次清除后生成宝箱（让玩家在建造阶段可以开启）
            GameSnapshot snap = engine.getSnapshot();
            if (snap.waveIndex >= 0 && !highlandPositions.empty()) {
                static std::mt19937 chestRng{std::random_device{}()};
                std::uniform_int_distribution<int> idxDist(0, static_cast<int>(highlandPositions.size()) - 1);
                chestManager.trySpawnChest(highlandPositions[idxDist(chestRng)], snap.waveIndex);
            }
        } else if (phaseAfter == GamePhase::GameOver) {
            audio.stopBGM();
            audio.playGameOver();
        } else if (phaseAfter == GamePhase::Victory) {
            audio.stopBGM();
            audio.playVictory();
        }
    }

    // 更新宝箱管理器
    GameSnapshot snap = engine.getSnapshot();
    chestManager.update(dt, snap.waveIndex, snap.totalWaveSpawns);

    // 处理宝箱事件
    if (chestManager.hasHellEvent()) {
        // 地狱模式：额外生成一个 Boss
        chestManager.clearHellEvent();
        // 注：这里需要在 WaveManager 中添加额外生成敌人的接口
        // 暂时只播放音效
        audio.playEnemyDeath(); // 复用音效
    }

    if (chestManager.hasRewardEvent()) {
        int gold = chestManager.getRewardGold();
        chestManager.clearRewardEvent();
        engine.addGold(gold);
    }

    if (chestManager.hasGambleResult()) {
        bool won = chestManager.getGambleWon();
        int gold = chestManager.getGambleGold();
        chestManager.clearGambleResult();
        if (won) {
            engine.addGold(gold);
        } else {
            // 扣除金币，但不小于 0
            engine.trySpend(gold);
        }
    }

    if (phaseAfter == GamePhase::GameOver) {
        currentScreen = Screen::GameOver;
    } else if (phaseAfter == GamePhase::Victory) {
        if (currentLevel < 4) {
            unlockedLevel = std::max(unlockedLevel, currentLevel + 1);
        }
        currentScreen = Screen::Victory;
    }
}

void GameFrontend::renderGame() {
    BeginDrawing();
    ClearBackground(Color{15, 15, 25, 255});

    drawMap(block);

    // Hover preview for tower placement
    GamePhase phase = engine.getPhase();
    bool canBuild = (phase == GamePhase::Build || phase == GamePhase::WaveCleared);
    if (canBuild && hoveredRow >= 0 && hoveredCol >= 0 && GetMousePosition().x < UI_PANEL_X) {
        if (block.canPlaceTower(hoveredRow, hoveredCol)) {
            Vector2 center = gridToScreen(hoveredRow, hoveredCol);
            Color prevColor = towerColor(towerName(selectedTowerKind));
            DrawRectangle(static_cast<int>(center.x - TILE_SIZE / 2 + 2),
                          static_cast<int>(center.y - TILE_SIZE / 2 + 2),
                          TILE_SIZE - 4, TILE_SIZE - 4,
                          Color{prevColor.r, prevColor.g, prevColor.b, 80});

            // Show range for hovered placement
            if (selectedTowerKind == TowerKind::Coffee)
                DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 92, Color{255,255,255,60});
            else if (selectedTowerKind == TowerKind::AI)
                DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 205, Color{255,255,255,60});
            else if (selectedTowerKind == TowerKind::Library)
                DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 228, Color{255,255,255,60});
            else if (selectedTowerKind == TowerKind::Class)
                DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 172, Color{255,255,255,60});
            else if (selectedTowerKind == TowerKind::Bilibili) {
                DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 305, Color{255,255,255,60});
                // Draw fire direction arrow
                Vector2 end = {center.x + bilibiliDir.x * 40, center.y + bilibiliDir.y * 40};
                DrawLine(static_cast<int>(center.x), static_cast<int>(center.y),
                         static_cast<int>(end.x), static_cast<int>(end.y), PINK);
                DrawCircle(static_cast<int>(end.x), static_cast<int>(end.y), 5, PINK);
            }
        }
    }

    drawTowers(engine.getTowers(), selectedTowerIndex);

    if (phase == GamePhase::WaveRunning) {
        drawEnemies(engine.getWaveManager().getLiveEnemies());
    }

    // 绘制宝箱
    drawChests(chestManager.getActiveChests());

    GameSnapshot snap = engine.getSnapshot();
    drawUI(snap, engine.getGold(), selectedTowerKind,
           engine.getExerciseMode(), selectedTowerIndex, showExerciseGuide,
           engine.getTimeScale());

    // Overlays
    if (currentScreen == Screen::GameOver) {
        drawGameOver(gameOverMenuSelection);
        if (IsKeyPressed(KEY_UP)) {
            gameOverMenuSelection = 0;
        }
        if (IsKeyPressed(KEY_DOWN)) {
            gameOverMenuSelection = 1;
        }
        if (IsKeyPressed(KEY_ENTER)) {
            if (gameOverMenuSelection == 0) {
                // Retry current level
                retryCurrentLevel();
            } else {
                // Return to level select without redoing the ASTI test.
                initMap();
                selectedTowerIndex = -1;
                showExerciseGuide = false;
                engine = GameEngine();
                chestManager.reset();
                currentScreen = Screen::LevelSelect;
            }
            gameOverMenuSelection = 0;
        }
    } else if (currentScreen == Screen::Victory) {
        bool hasNextLevel = (currentLevel < 4);
        drawVictory(victoryMenuSelection, hasNextLevel);
        if (IsKeyPressed(KEY_UP)) {
            victoryMenuSelection = 0;
        }
        if (IsKeyPressed(KEY_DOWN)) {
            if (hasNextLevel) {
                victoryMenuSelection = 1;
            }
        }
        if (IsKeyPressed(KEY_ENTER)) {
            if (victoryMenuSelection == 0 && hasNextLevel) {
                // Continue to next level
                goToNextLevel();
            } else {
                // Return to level select without redoing the ASTI test.
                initMap();
                selectedTowerIndex = -1;
                showExerciseGuide = false;
                engine = GameEngine();
                chestManager.reset();
                currentScreen = Screen::LevelSelect;
            }
            victoryMenuSelection = 0;
        }
    }

    // 绘制宝箱效果提示
    if (chestManager.hasEffectMessage()) {
        const std::string& msg = chestManager.getEffectMessage();
        static std::string cachedMsg;
        static int cachedTextWidth = 0;
        if (cachedMsg != msg) {
            cachedMsg = msg;
            cachedTextWidth = measureTextF(msg.c_str(), 28);
        }
        int tx = SCREEN_WIDTH / 2 - cachedTextWidth / 2;
        int ty = SCREEN_HEIGHT / 2 - 40;

        // 背景半透明黑框
        DrawRectangle(tx - 20, ty - 10, cachedTextWidth + 40, 50, Color{0, 0, 0, 180});
        DrawRectangleLines(tx - 20, ty - 10, cachedTextWidth + 40, 50, Color{255, 200, 50, 200});
        drawTextF(msg.c_str(), tx, ty, 28, Color{255, 220, 100, 255});
    }

    EndDrawing();
}

void GameFrontend::runGame() {
    float dt = GetFrameTime();

    if (currentScreen == Screen::Game) {
        GamePhase phase = engine.getPhase();
        if (phase == GamePhase::Build || phase == GamePhase::WaveCleared) {
            handleBuildInput();
        } else if (phase == GamePhase::WaveRunning) {
            handleWaveRunningInput();
        }
        handleGlobalInput();
        updateGame(dt);
    }

    renderGame();
}

}  // namespace frontend
