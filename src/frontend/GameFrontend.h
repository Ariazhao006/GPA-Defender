#pragma once

#include "gpa_defender/Block.h"
#include "gpa_defender/GameEngine.h"
#include "gpa_defender/LevelData.h"
#include "gpa_defender/Questionnaire.h"
#include "gpa_defender/Vector2D.h"
#include "gpa_defender/WaveManager.h"
#include "frontend/AudioManager.h"
#include "frontend/ChestManager.h"
#include "frontend/TextureManager.h"
#include "raylib.h"
#include <vector>
#include <string>

namespace frontend {

enum class Screen { MainMenu, Questionnaire, AstiSummary, LevelSelect, Game, GameOver, Victory };

class GameFrontend {
public:
    void run();

private:
    Block block{80.0f};
    GameEngine engine;
    std::vector<std::vector<Vector2D>> wavePaths;

    Questionnaire questionnaire;
    std::vector<int> answers;
    AstiResult astiResult;
    int currentQuestion = 0;

    TowerKind selectedTowerKind = TowerKind::Coffee;
    int selectedTowerIndex = -1;
    bool showExerciseGuide = false;
    int hoveredRow = -1;
    int hoveredCol = -1;
    Vector2D bilibiliDir{1.0f, 0.0f};
    int bilibiliDirIndex = 0;

    Screen currentScreen = Screen::MainMenu;
    Font uiFont;
    AudioManager audio;
    int prevEnemyCount = 0;
    GamePhase prevPhase = GamePhase::PreGame;

    int currentLevel = 1;           // 1-4 对应大一到大四
    int unlockedLevel = 1;
    int gameOverMenuSelection = 0;  // 0 = 重试本关, 1 = 返回主菜单
    int victoryMenuSelection = 0;   // 0 = 进入下一关/返回主菜单, 1 = 返回主菜单

    ChestManager chestManager;
    TextureManager textureManager;
    std::vector<Vector2D> highlandPositions;

    void loadLevelDefinition(int level);
    void initEngine();

    void retryCurrentLevel();
    void goToNextLevel();
    void startLevel(int level);

    void runMainMenu();
    void runQuestionnaire();
    void runAstiSummary();
    void runLevelSelect();
    void runGame();

    void handleBuildInput();
    void handleWaveRunningInput();
    void handleGlobalInput();
    void updateGame(float dt);
    void renderGame();
    void detectAudioEvents();

    bool tryPlaceSelectedTower(int row, int col);
    const char* towerName(TowerKind kind) const;
};

}  // namespace frontend
