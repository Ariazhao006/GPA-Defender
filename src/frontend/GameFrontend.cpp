#include "frontend/GameFrontend.h"
#include "frontend/Renderer.h"

#include "gpa_defender/Block.h"
#include "gpa_defender/DefenseTower.h"
#include "gpa_defender/Enemy.h"
#include "gpa_defender/LevelData.h"
#include "gpa_defender/PlayerStats.h"
#include "gpa_defender/WaveManager.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace frontend {

const char* GameFrontend::towerName(TowerKind kind) const {
    return GameEngine::towerSpec(kind).name;
}

void GameFrontend::loadLevelDefinition(int level) {
    currentLevel = clampLevelIndex(level);
    const LevelDefinition& levelDef = getLevelDefinition(currentLevel);

    block.buildBlocks(levelDef.map);

    highlandPositions.clear();
    const auto& grid = block.getGrid();
    for (int r = 0; r < block.getRows(); ++r) {
        for (int c = 0; c < block.getCols(); ++c) {
            if (grid[r][c].type == TileType::Highland) {
                highlandPositions.push_back(block.getBlockCenter(r, c));
            }
        }
    }

    wavePaths.clear();
    wavePaths.reserve(levelDef.pathTiles.size());
    for (const auto& path : levelDef.pathTiles) {
        std::vector<Vector2D> worldPath;
        worldPath.reserve(path.size());
        for (const GridCoord& tile : path) {
            worldPath.push_back(block.getBlockCenter(tile.row, tile.col));
        }
        wavePaths.push_back(std::move(worldPath));
    }

    engine = GameEngine(levelDef.waves, wavePaths, levelDef.startingGold);
}

void GameFrontend::startLevel(int level) {
    loadLevelDefinition(level);
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
    if (currentLevel < maxLevelCount()) {
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

    // Fix working directory when launched from file explorer (build/bin/)
    // The assets/ folder is relative to the project root.
    ChangeDirectory(TextFormat("%s/../..", GetApplicationDirectory()));

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

    textureManager.loadAll();

    loadLevelDefinition(currentLevel);
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

    textureManager.unloadAll();
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
    drawMainMenu(&textureManager);
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
    const int cardW = 309;
    const int cardH = 500;
    const int gap = 70;
    const int totalW = cardW * 4 + gap * 3;
    const int startX = SCREEN_WIDTH / 2 - totalW / 2;
    const int cardY = 280;
    const Rectangle retryRect{
        SCREEN_WIDTH / 2.0f - 228.0f,
        1120.0f,
        455.0f,
        90.0f
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
    drawLevelSelect(unlockedLevel, hoveredLevel, &textureManager);
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
        int lx = UI_PANEL_X + 20;
        int w = UI_PANEL_WIDTH - 39;

        TowerKind kinds[] = {TowerKind::Coffee, TowerKind::AI, TowerKind::Library,
                            TowerKind::Class, TowerKind::Bilibili};

        // Y positions must match drawUI layout
        int yTowerStart = 457;
        int yExModeStart = 790;
        int yStartWaveStart = 854;

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            // Tower selection buttons
            for (int i = 0; i < 5; ++i) {
                Rectangle btn = {static_cast<float>(lx), static_cast<float>(yTowerStart + i * 60),
                                 static_cast<float>(w), 54.0f};
                if (CheckCollisionPointRec(mouse, btn)) {
                    audio.playClick();
                    selectedTowerKind = kinds[i];
                    selectedTowerIndex = -1;
                    showExerciseGuide = false;
                }
            }

            // Exercise mode toggle
            Rectangle exBtn = {static_cast<float>(lx), static_cast<float>(yExModeStart),
                               static_cast<float>(w), 54.0f};
            if (CheckCollisionPointRec(mouse, exBtn)) {
                audio.playClick();
                engine.setExerciseMode(!engine.getExerciseMode());
                showExerciseGuide = true;
            }

            // Start Wave button
            Rectangle swBtn = {static_cast<float>(lx), static_cast<float>(yStartWaveStart),
                               static_cast<float>(w), 72.0f};
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
        if (chestManager.tryOpenChest(mousePos, 39.0f)) {
            audio.playClick();
            return;
        }

        int row, col;
        if (screenToGrid(mouse, row, col)) {
            const auto& towers = engine.getTowers();
            int clickedTower = -1;
            float minDist = 39.0f;
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
        if (chestManager.tryOpenChest(mousePos, 39.0f)) {
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
        if (currentLevel < maxLevelCount()) {
            unlockedLevel = std::max(unlockedLevel, currentLevel + 1);
        }
        currentScreen = Screen::Victory;
    }
}

void GameFrontend::renderGame() {
    BeginDrawing();
    ClearBackground(Color{15, 15, 25, 255});

    drawMap(block, &textureManager);

    // Hover preview for tower placement
    GamePhase phase = engine.getPhase();
    bool canBuild = (phase == GamePhase::Build || phase == GamePhase::WaveCleared);
    if (canBuild && hoveredRow >= 0 && hoveredCol >= 0 && GetMousePosition().x < UI_PANEL_X) {
        drawHoverPreview(hoveredRow, hoveredCol, selectedTowerKind,
                         {static_cast<float>(bilibiliDir.x), static_cast<float>(bilibiliDir.y)},
                         block, &textureManager);
    }

    drawTowers(engine.getTowers(), selectedTowerIndex, &textureManager);

    if (phase == GamePhase::WaveRunning) {
        drawEnemies(engine.getWaveManager().getLiveEnemies(), &textureManager);
    }

    drawChests(chestManager.getActiveChests(), &textureManager);

    GameSnapshot snap = engine.getSnapshot();
    drawUI(snap, engine.getGold(), selectedTowerKind,
           engine.getExerciseMode(), selectedTowerIndex, showExerciseGuide,
           engine.getTimeScale(), &textureManager);

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
                selectedTowerIndex = -1;
                showExerciseGuide = false;
                engine = GameEngine();
                chestManager.reset();
                currentScreen = Screen::LevelSelect;
            }
            gameOverMenuSelection = 0;
        }
    } else if (currentScreen == Screen::Victory) {
        bool hasNextLevel = (currentLevel < maxLevelCount());
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
            cachedTextWidth = measureTextF(msg.c_str(), 36);
        }
        int tx = SCREEN_WIDTH / 2 - cachedTextWidth / 2;
        int ty = SCREEN_HEIGHT / 2 - 52;

        // 背景半透明黑框
        DrawRectangle(tx - 26, ty - 13, cachedTextWidth + 52, 65, Color{0, 0, 0, 180});
        DrawRectangleLines(tx - 26, ty - 13, cachedTextWidth + 52, 65, Color{255, 200, 50, 200});
        drawTextF(msg.c_str(), tx, ty, 36, Color{255, 220, 100, 255});
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
