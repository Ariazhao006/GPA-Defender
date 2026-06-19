#pragma once

#include "gpa_defender/Block.h"
#include "gpa_defender/DefenseTower.h"
#include "gpa_defender/Enemy.h"
#include "gpa_defender/GameEngine.h"
#include "gpa_defender/Questionnaire.h"
#include "raylib.h"
#include <vector>
#include <string>

namespace frontend {

constexpr int TILE_SIZE = 64;
constexpr int MAP_OFFSET_X = 36;
constexpr int MAP_OFFSET_Y = 36;
constexpr int MAP_COLS = 12;
constexpr int MAP_ROWS = 10;
constexpr int UI_PANEL_X = 828;
constexpr int UI_PANEL_WIDTH = 372;
constexpr int SCREEN_WIDTH = 1200;
constexpr int SCREEN_HEIGHT = 800;

Color tileColor(TileType type);
Color towerColor(const std::string& name);
Color enemyColor(const std::string& name);
Color indicatorColor(int idx);
Color phaseColor(GamePhase phase);

Vector2 gridToScreen(int row, int col);
bool screenToGrid(Vector2 screenPos, int& outRow, int& outCol);

// Font management for CJK text rendering
void setUiFont(Font font);
Font getUiFont();
int measureTextF(const char* text, int fontSize);
void drawTextF(const char* text, int x, int y, int fontSize, Color color);

void drawMap(const Block& block);
void drawTower(const DefenseTower& tower, bool showRange, bool selected);
void drawTowers(const std::vector<std::unique_ptr<DefenseTower>>& towers,
                int selectedIndex);
void drawEnemy(const Enemy& enemy);
void drawEnemies(const std::vector<Enemy*>& enemies);
void drawUI(const GameSnapshot& snap, int gold, TowerKind selectedTower,
            bool exerciseMode, int selectedTowerIndex, bool showExerciseGuide);
void drawMainMenu();
void drawGameOver();
void drawVictory();
void drawQuestionnaire(const Questionnaire& q, int current,
                       const std::vector<int>& answers);
void drawAstiSummary(const AstiResult& result);

}  // namespace frontend
