#pragma once

#include "gpa_defender/Block.h"
#include "gpa_defender/GameEngine.h"
#include "gpa_defender/Questionnaire.h"
#include "gpa_defender/Vector2D.h"
#include "gpa_defender/WaveManager.h"
#include "frontend/AudioManager.h"
#include "raylib.h"
#include <vector>
#include <string>

namespace frontend {

enum class Screen { MainMenu, Questionnaire, AstiSummary, Game, GameOver, Victory };

class GameFrontend {
public:
    void run();

private:
    Block block{64.0f};
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

    static const std::vector<std::vector<int>> MAP_DATA;

    void initMap();
    void initPaths();
    void initWaves();
    void initEngine();

    void runMainMenu();
    void runQuestionnaire();
    void runAstiSummary();
    void runGame();

    void handleBuildInput();
    void handleWaveRunningInput();
    void handleGlobalInput();
    void updateGame(float dt);
    void renderGame();
    void detectAudioEvents();

    bool tryPlaceSelectedTower(int row, int col);
    const char* towerName(TowerKind kind) const;
    int towerCost(TowerKind kind) const;
    const char* towerDesc(TowerKind kind) const;
};

}  // namespace frontend
