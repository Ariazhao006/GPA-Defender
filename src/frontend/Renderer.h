#pragma once

#include "gpa_defender/Block.h"
#include "gpa_defender/DefenseTower.h"
#include "gpa_defender/Enemy.h"
#include "gpa_defender/GameEngine.h"
#include "gpa_defender/Questionnaire.h"
#include "frontend/ChestManager.h"
#include "frontend/StageScore.h"
#include "raylib.h"
#include <cstddef>
#include <vector>
#include <string>

namespace frontend {

class TextureManager;

constexpr int TILE_SIZE = 104;
constexpr int MAP_OFFSET_X = 59;
constexpr int MAP_OFFSET_Y = 210;
constexpr int MAP_COLS = 12;
constexpr int MAP_ROWS = 10;
constexpr int UI_PANEL_X = 1346;
constexpr int UI_PANEL_WIDTH = 605;
constexpr int SCREEN_WIDTH = 1950;
constexpr int SCREEN_HEIGHT = 1300;

Color tileColor(TileType type);
Color towerColor(const std::string& name);
Color enemyColor(const std::string& name);
Color indicatorColor(int idx);
Color phaseColor(GamePhase phase);
Color chestColor(ChestType type);
const char* chestLabel(ChestType type);

Vector2 gridToScreen(int row, int col);
bool screenToGrid(Vector2 screenPos, int& outRow, int& outCol);

// Font management for CJK text rendering
void setUiFont(Font font);
const Font& getUiFont();
void initVirtualCanvas();
void shutdownVirtualCanvas();
void beginVirtualDrawing();
void endVirtualDrawing();
int measureTextF(const char* text, int fontSize);
void drawTextF(const char* text, int x, int y, int fontSize, Color color);

Rectangle mainMenuStartRect();
Rectangle mainMenuContinueRect();
Rectangle mainMenuNewGameRect(bool hasSavedGame);
Rectangle mainMenuSaveSlotsRect();
Rectangle questionnaireOptionRect(std::size_t index);
Rectangle astiContinueRect();
Rectangle gameOverOptionRect(int option);
Rectangle victoryOptionRect(int option, bool hasNextLevel);
Rectangle seedBarTowerRect(int index);
Rectangle levelSelectRetakeRect();
Rectangle levelSelectReturnRect();

// Helper: pick tile source rect from the map tilesheet (17x12 grid of 64x64)
Rectangle mapTileSrc(int col, int row);

// Helper: get the sprite name for a given tower/enemy
const char* towerSpriteName(const std::string& towerName);
const char* enemySpriteName(const std::string& enemyName);

// Sprite drawing helper
void drawSprite(const TextureManager& tm, const char* name,
                float cx, float cy, float scale, Color tint);

// ---- Core draw functions (with optional textures) ----
// Pass nullptr for tm to fall back to geometric shapes.

void drawMap(const Block& block, const TextureManager* tm = nullptr);
void drawBaseHealth(const Block& block, int hp, int maxHp);
void drawTower(const DefenseTower& tower, bool showRange, bool selected,
               const TextureManager* tm = nullptr);
void drawTowers(const std::vector<std::unique_ptr<DefenseTower>>& towers,
                int selectedIndex, const TextureManager* tm = nullptr);
void drawEnemy(const Enemy& enemy, const TextureManager* tm = nullptr);
void drawEnemies(const std::vector<Enemy*>& enemies,
                 const TextureManager* tm = nullptr);
void drawChests(const std::vector<Chest>& chests,
                const TextureManager* tm = nullptr);
void drawUI(const GameSnapshot& snap, int gold, int currentLevel, TowerKind selectedTower,
            bool exerciseMode, int selectedTowerIndex, bool showExerciseGuide,
            float timeScale, float panelScrollOffset, const TextureManager* tm);
void drawSeedBar(int gold, TowerKind selectedTower, const TextureManager* tm);
void drawMainMenu(const TextureManager* tm = nullptr, bool hasSavedGame = false);
void drawLevelSelect(int unlockedLevel, int hoveredLevel,
                     const TextureManager* tm = nullptr);
void drawGameOver(int selection);
void drawVictory(int selection, bool hasNextLevel,
                 const StageScoreRecord* currentScore,
                 const std::vector<StageScoreRecord>& stageScores);
void drawQuestionnaire(const Questionnaire& q, int current,
                       const std::vector<int>& answers);
void drawAstiSummary(const AstiResult& result);

// ---- Hover preview helpers ----
void drawHoverPreview(int hoveredRow, int hoveredCol,
                      TowerKind selectedTower, const Vector2& bilibiliDir,
                      const Block& block, const TextureManager* tm = nullptr);

}  // namespace frontend
