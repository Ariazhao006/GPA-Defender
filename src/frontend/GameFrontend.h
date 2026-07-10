#pragma once

#include "gpa_defender/Block.h"
#include "gpa_defender/GameEngine.h"
#include "gpa_defender/LevelData.h"
#include "gpa_defender/Questionnaire.h"
#include "gpa_defender/Vector2D.h"
#include "gpa_defender/WaveManager.h"
#include "frontend/AudioManager.h"
#include "frontend/EffectManager.h"
#include "frontend/SettlementVideoPlayer.h"
#include "frontend/StageScore.h"
#include "frontend/TextureManager.h"
#include "raylib.h"
#include <array>
#include <vector>
#include <string>

namespace frontend {

enum class Screen { MainMenu, SaveName, SaveSlots, OpeningVideo, Questionnaire, AstiSummary, TowerIntroVideo, LevelSelect, Game, GameOver, GraduationVideo, Victory };

struct SaveSlotInfo {
    bool occupied = false;
    int slot = 0;
    std::string name;
    std::string timestamp;
    int level = 1;
    int unlockedLevel = 1;
    int waveIndex = -1;
    GamePhase phase = GamePhase::Build;
};

class GameFrontend {
public:
    void run();

private:
    Block block{104.0f};
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
    float uiScrollOffset = 0.0f;
    Vector2D bilibiliDir{1.0f, 0.0f};
    int bilibiliDirIndex = 0;

    Screen currentScreen = Screen::MainMenu;
    Font uiFont;
    AudioManager audio;
    int prevEnemyCount = 0;
    GamePhase prevPhase = GamePhase::PreGame;

    int currentLevel = 1;           // 1-4 对应大一到大四
    int unlockedLevel = 1;
    int gameOverMenuSelection = 0;  // 0 = retry, 1 = level select, 2 = main menu
    int victoryMenuSelection = 0;   // with next: 0 = next, 1 = level select, 2 = main menu
    bool mouseClickBlockedUntilRelease = false;
    bool savedGameAvailable = false;
    int currentSaveSlot = -1;
    int pendingOverwriteSlot = -1;
    int saveSlotSelection = 0;
    bool saveSlotsForNewGame = false;
    std::string saveNameInput;
    std::array<SaveSlotInfo, 5> saveSlots;
    std::vector<StageScoreRecord> stageScores;
    bool stageScoreRecordedForCurrentLevel = false;
    bool graduationVideoPlayedForCurrentLevel = false;
    std::string statusBannerText;
    float statusBannerTimer = 0.0f;
    std::string chestEffectMessage;
    float chestEffectDisplayTimer = 0.0f;

    EffectManager effectManager;
    SettlementVideoPlayer openingVideo;
    SettlementVideoPlayer towerIntroVideo;
    SettlementVideoPlayer graduationVideo;
    TextureManager textureManager;
    std::vector<Vector2D> highlandPositions;

    void loadLevelDefinition(int level);
    void initEngine();

    void retryCurrentLevel();
    void goToNextLevel();
    void startLevel(int level);

    void runMainMenu();
    void runSaveName();
    void runSaveSlots();
    void runOpeningVideo();
    void runQuestionnaire();
    void runAstiSummary();
    void runTowerIntroVideo();
    void runLevelSelect();
    void runGame();

    void handleBuildInput();
    void handleWaveRunningInput();
    void handleGlobalInput();
    void updateGame(float dt);
    void renderGame();
    void startOpeningVideo();
    void updateOpeningVideo(float dt);
    void renderOpeningVideo();
    void startTowerIntroVideo();
    void updateTowerIntroVideo(float dt);
    void renderTowerIntroVideo();
    void startGraduationVideo();
    void updateGraduationVideo(float dt);
    void renderGraduationVideo();
    void detectAudioEvents();
    void handleChestEvents();

    bool tryPlaceSelectedTower(int row, int col);
    const char* towerName(TowerKind kind) const;
    bool primaryClickPressed() const;
    void blockMouseClickUntilRelease();
    void updateMouseClickBlock();
    void showStatusBanner(const std::string& text);
    void startNewGameFlow();
    void returnToMainMenuWithSave();
    void continueSavedGame();
    void refreshSaveSlots();
    bool writeCurrentSave();
    bool loadSaveSlot(int slot);
    StageScoreRecord makeStageScoreRecord() const;
    void recordCurrentStageScore();
    const StageScoreRecord* currentStageScore() const;
    int latestSaveSlot() const;
    int firstEmptySaveSlot() const;
    void beginNewSaveNaming(int overwriteSlot);
    void drawSaveNameScreen();
    void drawSaveSlotsScreen();
};

}  // namespace frontend
