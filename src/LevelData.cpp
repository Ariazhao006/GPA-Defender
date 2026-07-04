#include "gpa_defender/LevelData.h"

#include <array>
#include <algorithm>
#include <utility>

namespace {

using MapData = std::vector<std::vector<int>>;
using PathData = std::vector<std::vector<GridCoord>>;

WaveDefinition wave(std::vector<SpawnEvent> spawns, int clearBonus) {
    WaveDefinition out;
    out.spawns = std::move(spawns);
    out.clearBonus = clearBonus;
    return out;
}

LevelDefinition makeLevel1() {
    return {
        1,
        300,
        {
            {  3, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2 },
            {  2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2 },
            {  2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2 },
            {  2, 2, 2, 1, 1, 1, 1, 1, 1, 2, 2, 2 },
            {  2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2 },
            {  2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2 },
            {  2, 2, 2, 1, 1, 1, 1, 1, 1, 2, 2, 2 },
            {  2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2 },
            {  2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 4 },
            {  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
        },
        {{
            {0, 0}, {0, 3}, {3, 3}, {3, 8},
            {6, 8}, {6, 3}, {8, 3}, {8, 11},
        }},
        {
            wave({{0.0f, EnemyKind::Subject, 0},
                  {3.0f, EnemyKind::Subject, 0},
                  {6.0f, EnemyKind::Social, 0}}, 50),
            wave({{0.0f, EnemyKind::Subject, 0},
                  {2.0f, EnemyKind::Subject, 0},
                  {3.5f, EnemyKind::Social, 0},
                  {5.0f, EnemyKind::Subject, 0},
                  {6.5f, EnemyKind::Social, 0},
                  {8.0f, EnemyKind::MorningClass, 0}}, 90),
            wave({{0.0f, EnemyKind::Subject, 0},
                  {1.5f, EnemyKind::Social, 0},
                  {3.0f, EnemyKind::MorningClass, 0},
                  {4.5f, EnemyKind::Subject, 0},
                  {6.0f, EnemyKind::Social, 0},
                  {7.5f, EnemyKind::MorningClass, 0},
                  {9.0f, EnemyKind::MidtermBoss, 0}}, 170),
        },
    };
}

LevelDefinition makeLevel2() {
    return {
        2,
        450,
        {
            {  3, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
            {  2, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2 },
            {  2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2 },
            {  2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2 },
            {  2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2 },
            {  2, 3, 1, 1, 1, 1, 2, 2, 2, 1, 2, 2 },
            {  2, 2, 2, 2, 2, 1, 1, 1, 2, 1, 2, 2 },
            {  2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2 },
            {  2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 4 },
            {  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
        },
        {
            {{0, 0}, {0, 2}, {1, 2}, {1, 4}, {3, 4},
             {3, 7}, {4, 7}, {4, 9}, {8, 9}, {8, 11}},
            {{5, 1}, {5, 5}, {6, 5}, {6, 7},
             {7, 7}, {7, 9}, {8, 9}, {8, 11}},
        },
        {
            wave({{0.0f, EnemyKind::Subject, 0},
                  {2.0f, EnemyKind::MorningClass, 1},
                  {5.0f, EnemyKind::Social, 0},
                  {8.0f, EnemyKind::Research, 1}}, 60),
            wave({{0.0f, EnemyKind::Subject, 1},
                  {1.5f, EnemyKind::Subject, 0},
                  {3.0f, EnemyKind::Research, 0},
                  {4.5f, EnemyKind::MorningClass, 1},
                  {6.0f, EnemyKind::ShortVideo, 0},
                  {8.0f, EnemyKind::Research, 1}}, 105),
            wave({{0.0f, EnemyKind::Research, 0},
                  {1.4f, EnemyKind::Subject, 1},
                  {2.8f, EnemyKind::GroupProject, 1},
                  {4.2f, EnemyKind::ShortVideo, 0},
                  {5.6f, EnemyKind::PeerPressure, 0},
                  {7.0f, EnemyKind::Research, 1},
                  {9.0f, EnemyKind::MidtermBoss, 1}}, 195),
        },
    };
}

LevelDefinition makeLevel3() {
    PathData paths = {
        {{0, 0}, {0, 3}, {1, 3}, {2, 3}, {2, 6}, {3, 6}, {3, 7}, {3, 8},
         {4, 8}, {5, 8}, {5, 7}, {5, 6}, {5, 5}, {5, 4}, {5, 3}, {5, 2},
         {6, 2}, {7, 2}, {7, 3}, {7, 4}, {7, 5}, {7, 6}, {7, 7}, {7, 8},
         {8, 8}, {8, 9}, {8, 10}, {8, 11}},
        {{0, 0}, {0, 3}, {1, 3}, {2, 3}, {2, 4}, {2, 5}, {2, 6}, {3, 6},
         {3, 7}, {3, 8}, {4, 8}, {5, 8}, {5, 7}, {5, 6}, {5, 5}, {5, 4},
         {5, 3}, {5, 2}, {6, 2}, {7, 2}, {7, 3}, {7, 4}, {7, 5}, {7, 6},
         {7, 7}, {7, 8}, {8, 8}, {8, 9}, {8, 10}, {8, 11}},
    };

    return {
        3,
        600,
        {
            {  3, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2 },
            {  2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2 },
            {  2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 2 },
            {  2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2, 2 },
            {  2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2 },
            {  2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2 },
            {  2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
            {  2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2 },
            {  2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 4 },
            {  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
        },
        paths,
        {
            wave({{0.0f, EnemyKind::Subject, 0},
                  {2.0f, EnemyKind::Research, 1},
                  {5.0f, EnemyKind::GroupProject, 0},
                  {8.0f, EnemyKind::MorningClass, 1}}, 70),
            wave({{0.0f, EnemyKind::ExamSyllabus, 1},
                  {1.3f, EnemyKind::Social, 0},
                  {2.6f, EnemyKind::PeerPressure, 0},
                  {3.9f, EnemyKind::Research, 1},
                  {5.2f, EnemyKind::ShortVideo, 0},
                  {6.8f, EnemyKind::ExamSyllabus, 1},
                  {8.4f, EnemyKind::Social, 0}}, 120),
            wave({{0.0f, EnemyKind::GroupProject, 0},
                  {1.2f, EnemyKind::Research, 1},
                  {2.4f, EnemyKind::ExamSyllabus, 1},
                  {3.6f, EnemyKind::PeerPressure, 0},
                  {4.8f, EnemyKind::GroupProject, 0},
                  {6.0f, EnemyKind::ExamSyllabus, 1},
                  {7.2f, EnemyKind::PeerPressure, 0},
                  {9.0f, EnemyKind::MidtermBoss, 1}}, 225),
        },
    };
}

LevelDefinition makeLevel4() {
    return {
        4,
        750,
        {
            {  3, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2 },
            {  2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2 },
            {  2, 2, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2 },
            {  2, 2, 2, 2, 2, 1, 1, 1, 2, 2, 2, 2 },
            {  2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2 },
            {  2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2 },
            {  2, 2, 2, 1, 2, 2, 1, 1, 1, 1, 2, 2 },
            {  2, 2, 2, 1, 2, 2, 2, 2, 2, 1, 2, 2 },
            {  2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 4 },
            {  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
        },
        {
            {{0, 0}, {0, 1}, {0, 2}, {0, 3}, {1, 3}, {2, 3}, {2, 4}, {2, 5},
             {3, 5}, {3, 6}, {3, 7}, {4, 7}, {4, 8}, {4, 9}, {5, 9}, {5, 8},
             {5, 7}, {5, 6}, {5, 5}, {5, 4}, {5, 3}, {6, 3}, {7, 3}, {7, 4},
             {7, 5}, {7, 6}, {7, 7}, {7, 8}, {7, 9}, {8, 9}, {8, 10}, {8, 11}},
            {{0, 0}, {0, 1}, {0, 2}, {0, 3}, {1, 3}, {2, 3}, {2, 4}, {2, 5},
             {3, 5}, {3, 6}, {3, 7}, {4, 7}, {4, 8}, {4, 9}, {5, 9}, {5, 8},
             {5, 7}, {5, 6}, {6, 6}, {6, 7}, {6, 8}, {6, 9}, {7, 9}, {8, 9},
             {8, 10}, {8, 11}},
        },
        {
            wave({{0.0f, EnemyKind::Subject, 0},
                  {1.5f, EnemyKind::Subject, 1},
                  {3.0f, EnemyKind::Research, 0},
                  {5.0f, EnemyKind::Social, 1},
                  {7.0f, EnemyKind::MorningClass, 0}}, 80),
            wave({{0.0f, EnemyKind::GroupProject, 1},
                  {1.2f, EnemyKind::Research, 1},
                  {2.4f, EnemyKind::ExamSyllabus, 0},
                  {3.6f, EnemyKind::PeerPressure, 1},
                  {4.8f, EnemyKind::ShortVideo, 0},
                  {6.0f, EnemyKind::Research, 1},
                  {7.2f, EnemyKind::ExamSyllabus, 0},
                  {8.8f, EnemyKind::PeerPressure, 1}}, 135),
            wave({{0.0f, EnemyKind::MidtermBoss, 0},
                  {1.2f, EnemyKind::GroupProject, 1},
                  {2.4f, EnemyKind::ExamSyllabus, 0},
                  {3.6f, EnemyKind::PeerPressure, 1},
                  {4.8f, EnemyKind::ShortVideo, 0},
                  {6.0f, EnemyKind::GroupProject, 1},
                  {7.2f, EnemyKind::ExamSyllabus, 0},
                  {8.4f, EnemyKind::PeerPressure, 1},
                  {10.0f, EnemyKind::MidtermBoss, 0}}, 250),
        },
    };
}

const std::array<LevelDefinition, 4>& levels() {
    static const std::array<LevelDefinition, 4> data = {
        makeLevel1(),
        makeLevel2(),
        makeLevel3(),
        makeLevel4(),
    };
    return data;
}

} // namespace

int clampLevelIndex(int level) {
    return std::clamp(level, 1, maxLevelCount());
}

int maxLevelCount() {
    return static_cast<int>(levels().size());
}

const LevelDefinition& getLevelDefinition(int level) {
    return levels()[static_cast<std::size_t>(clampLevelIndex(level) - 1)];
}
