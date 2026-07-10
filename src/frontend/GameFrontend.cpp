#include "frontend/GameFrontend.h"
#include "frontend/AssetPaths.h"
#include "frontend/Renderer.h"

#include "gpa_defender/Block.h"
#include "gpa_defender/DefenseTower.h"
#include "gpa_defender/Enemy.h"
#include "gpa_defender/LevelData.h"
#include "gpa_defender/PlayerStats.h"
#include "gpa_defender/WaveManager.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace frontend {

namespace {

constexpr int kUiFontBaseSize = 96;
constexpr int kFallbackWindowWidth = 1280;
constexpr int kFallbackWindowHeight = 853;

bool usableLoadedFont(const Font& font, const Font& defaultFont) {
    return IsFontReady(font) && font.texture.id != 0 && font.texture.id != defaultFont.texture.id;
}

Font loadUiFont(const int* codepoints, int cpCount) {
    const Font defaultFont = GetFontDefault();
    const std::array<const char*, 7> candidates = {
        "C:\\Windows\\Fonts\\msyhbd.ttc",
        "C:\\Windows\\Fonts\\Dengb.ttf",
        "C:\\Windows\\Fonts\\simhei.ttf",
        "C:\\Windows\\Fonts\\msyh.ttc",
        "C:\\Windows\\Fonts\\NotoSansSC-VF.ttf",
        "C:\\Windows\\Fonts\\Deng.ttf",
        "C:\\Windows\\Fonts\\simsun.ttc"
    };

    for (const char* path : candidates) {
        if (!FileExists(path)) continue;
        Font font = LoadFontEx(path, kUiFontBaseSize, const_cast<int*>(codepoints), cpCount);
        if (usableLoadedFont(font, defaultFont)) {
            TraceLog(LOG_INFO, "Loaded UI font: %s", path);
            return font;
        }
        if (IsFontReady(font) && font.texture.id != 0 &&
            font.texture.id != defaultFont.texture.id) {
            UnloadFont(font);
        }
        TraceLog(LOG_WARNING, "Skipped unusable UI font: %s", path);
    }

    TraceLog(LOG_WARNING, "Falling back to raylib default font; Chinese glyphs may be missing.");
    return defaultFont;
}

std::filesystem::path saveDir() {
#ifdef GPA_DEFENDER_PROJECT_ROOT
    return std::filesystem::path(GPA_DEFENDER_PROJECT_ROOT) / "saves";
#else
    return std::filesystem::current_path() / "saves";
#endif
}

void fitWindowToCurrentMonitor() {
    const int monitor = GetCurrentMonitor();
    const int monitorW = GetMonitorWidth(monitor);
    const int monitorH = GetMonitorHeight(monitor);
    const int maxW = std::max(640, monitorW - 80);
    const int maxH = std::max(480, monitorH - 180);
    const float fitScale = std::min({
        1.0f,
        maxW / static_cast<float>(SCREEN_WIDTH),
        maxH / static_cast<float>(SCREEN_HEIGHT)
    });
    const int minW = std::min(960, maxW);
    const int minH = std::min(640, maxH);
    const int windowW = std::max(minW, static_cast<int>(std::round(SCREEN_WIDTH * fitScale)));
    const int windowH = std::max(minH, static_cast<int>(std::round(SCREEN_HEIGHT * fitScale)));

    SetWindowMinSize(minW, minH);
    SetWindowSize(windowW, windowH);

    const Vector2 monitorPos = GetMonitorPosition(monitor);
    const int x = static_cast<int>(monitorPos.x) + std::max(0, (monitorW - windowW) / 2);
    const int y = static_cast<int>(monitorPos.y) + std::max(20, (monitorH - windowH) / 2);
    SetWindowPosition(x, y);
}

std::filesystem::path savePath(int slot) {
    return saveDir() / ("slot" + std::to_string(slot + 1) + ".sav");
}

std::string sanitizeSaveName(const std::string& input) {
    std::string out;
    for (char ch : input) {
        unsigned char c = static_cast<unsigned char>(ch);
        if (c < 32 || ch == '=' || ch == '|' || ch == ',' || ch == '\r' || ch == '\n') continue;
        out.push_back(ch);
        if (out.size() >= 32) break;
    }
    while (!out.empty() && out.front() == ' ') out.erase(out.begin());
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out.empty() ? "Untitled Save" : out;
}

std::string nowTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    std::ostringstream oss;
    oss << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

int phaseToInt(GamePhase phase) {
    return static_cast<int>(phase);
}

GamePhase intToPhase(int value) {
    if (value < static_cast<int>(GamePhase::PreGame) || value > static_cast<int>(GamePhase::Victory)) {
        return GamePhase::Build;
    }
    return static_cast<GamePhase>(value);
}

int towerKindToInt(TowerKind kind) {
    return static_cast<int>(kind);
}

TowerKind intToTowerKind(int value) {
    if (value < static_cast<int>(TowerKind::Coffee) || value > static_cast<int>(TowerKind::Bilibili)) {
        return TowerKind::Coffee;
    }
    return static_cast<TowerKind>(value);
}

std::vector<std::string> splitString(const std::string& value, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(value);
    std::string part;
    while (std::getline(ss, part, delim)) {
        if (!part.empty()) parts.push_back(part);
    }
    return parts;
}

std::string joinStrings(const std::vector<std::string>& values, char delim) {
    std::string out;
    for (const std::string& value : values) {
        if (!out.empty()) out.push_back(delim);
        out += value;
    }
    return out;
}

std::string joinAnswers(const std::vector<int>& values) {
    std::string out;
    for (int value : values) {
        if (!out.empty()) out.push_back(',');
        out += std::to_string(value);
    }
    return out;
}

std::vector<int> parseAnswers(const std::string& value) {
    std::vector<int> out;
    for (const std::string& part : splitString(value, ',')) {
        out.push_back(std::atoi(part.c_str()));
    }
    return out;
}

std::string serializeStageScore(const StageScoreRecord& record) {
    std::ostringstream out;
    out << record.level << ","
        << record.score << ","
        << record.gold << ","
        << record.baseHp << ","
        << record.baseMaxHp << ","
        << record.academic << ","
        << record.physical << ","
        << record.mental << ","
        << record.connection << ","
        << record.waveIndex << ","
        << record.thresholdAcademic << ","
        << record.thresholdPhysical << ","
        << record.thresholdMental << ","
        << record.thresholdConnection << ","
        << record.timestamp;
    return out.str();
}

bool parseStageScore(const std::string& value, StageScoreRecord& out) {
    std::vector<std::string> parts = splitString(value, ',');
    if (parts.size() < 10) return false;
    out.level = std::atoi(parts[0].c_str());
    out.score = std::atoi(parts[1].c_str());
    out.gold = std::atoi(parts[2].c_str());
    out.baseHp = std::atoi(parts[3].c_str());
    out.baseMaxHp = std::atoi(parts[4].c_str());
    out.academic = std::atoi(parts[5].c_str());
    out.physical = std::atoi(parts[6].c_str());
    out.mental = std::atoi(parts[7].c_str());
    out.connection = std::atoi(parts[8].c_str());
    out.waveIndex = std::atoi(parts[9].c_str());
    if (parts.size() >= 15) {
        out.thresholdAcademic = std::atoi(parts[10].c_str());
        out.thresholdPhysical = std::atoi(parts[11].c_str());
        out.thresholdMental = std::atoi(parts[12].c_str());
        out.thresholdConnection = std::atoi(parts[13].c_str());
        out.timestamp = parts[14];
    } else {
        out.timestamp = parts.size() >= 11 ? parts[10] : "";
    }
    if (out.level < 1) out.level = 1;
    if (out.baseMaxHp <= 0) out.baseMaxHp = 100;
    if (out.thresholdAcademic <= 0) out.thresholdAcademic = 50;
    if (out.thresholdPhysical <= 0) out.thresholdPhysical = 50;
    if (out.thresholdMental <= 0) out.thresholdMental = 50;
    if (out.thresholdConnection <= 0) out.thresholdConnection = 50;
    return true;
}

} // namespace

const char* GameFrontend::towerName(TowerKind kind) const {
    return GameEngine::towerSpec(kind).name;
}

bool GameFrontend::primaryClickPressed() const {
    return !mouseClickBlockedUntilRelease && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void GameFrontend::blockMouseClickUntilRelease() {
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        mouseClickBlockedUntilRelease = true;
    }
}

void GameFrontend::updateMouseClickBlock() {
    if (mouseClickBlockedUntilRelease && !IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        mouseClickBlockedUntilRelease = false;
    }
}

void GameFrontend::showStatusBanner(const std::string& text) {
    statusBannerText = text;
    statusBannerTimer = 2.15f;
}

StageScoreRecord GameFrontend::makeStageScoreRecord() const {
    GameSnapshot snapshot = engine.getSnapshot();
    StageScoreRecord record;
    record.level = currentLevel;
    record.gold = snapshot.gold;
    record.baseHp = snapshot.baseHp;
    record.baseMaxHp = snapshot.baseMaxHp;
    record.academic = snapshot.currentAcademic;
    record.physical = snapshot.currentPhysical;
    record.mental = snapshot.currentMental;
    record.connection = snapshot.currentConnection;
    record.thresholdAcademic = snapshot.thresholdAcademic;
    record.thresholdPhysical = snapshot.thresholdPhysical;
    record.thresholdMental = snapshot.thresholdMental;
    record.thresholdConnection = snapshot.thresholdConnection;
    record.waveIndex = snapshot.waveIndex;
    record.timestamp = nowTimestamp();

    const int survivalScore = record.baseMaxHp > 0
        ? record.baseHp * 1000 / record.baseMaxHp
        : 0;
    const int statScore = (record.academic + record.physical +
                           record.mental + record.connection) * 5;
    const int levelBonus = record.level * 500;
    record.score = std::max(0, survivalScore + statScore + record.gold + levelBonus);
    return record;
}

void GameFrontend::recordCurrentStageScore() {
    StageScoreRecord record = makeStageScoreRecord();
    for (StageScoreRecord& existing : stageScores) {
        if (existing.level == record.level) {
            existing = record;
            stageScoreRecordedForCurrentLevel = true;
            return;
        }
    }
    stageScores.push_back(record);
    std::sort(stageScores.begin(), stageScores.end(),
              [](const StageScoreRecord& a, const StageScoreRecord& b) {
                  return a.level < b.level;
              });
    stageScoreRecordedForCurrentLevel = true;
}

const StageScoreRecord* GameFrontend::currentStageScore() const {
    for (const StageScoreRecord& record : stageScores) {
        if (record.level == currentLevel) return &record;
    }
    return nullptr;
}

void GameFrontend::startNewGameFlow() {
    audio.playClick();
    int slot = pendingOverwriteSlot >= 0 ? pendingOverwriteSlot : firstEmptySaveSlot();
    if (slot < 0) slot = 0;
    currentSaveSlot = slot;
    saveSlots[static_cast<std::size_t>(slot)].name = sanitizeSaveName(saveNameInput);
    saveSlots[static_cast<std::size_t>(slot)].occupied = true;
    currentQuestion = 0;
    currentLevel = 1;
    unlockedLevel = 1;
    selectedTowerKind = TowerKind::Coffee;
    selectedTowerIndex = -1;
    showExerciseGuide = false;
    hoveredRow = -1;
    hoveredCol = -1;
    uiScrollOffset = 0.0f;
    bilibiliDir = {1.0f, 0.0f};
    bilibiliDirIndex = 0;
    gameOverMenuSelection = 0;
    victoryMenuSelection = 0;
    savedGameAvailable = true;
    stageScores.clear();
    stageScoreRecordedForCurrentLevel = false;
    graduationVideoPlayedForCurrentLevel = false;
    pendingOverwriteSlot = -1;
    statusBannerTimer = 0.0f;
    statusBannerText.clear();
    questionnaire = buildAstiQuestionnaire();
    answers.assign(questionnaire.getQuestions().size(), -1);
    engine = GameEngine();
    chestEffectMessage.clear();
    chestEffectDisplayTimer = 0.0f;
    effectManager.clear();
    audio.stopBGM();
    startOpeningVideo();
}

void GameFrontend::returnToMainMenuWithSave() {
    writeCurrentSave();
    refreshSaveSlots();
    selectedTowerIndex = -1;
    showExerciseGuide = false;
    statusBannerTimer = 0.0f;
    statusBannerText.clear();
    audio.stopBGM();
    currentScreen = Screen::MainMenu;
}

void GameFrontend::continueSavedGame() {
    if (!savedGameAvailable) return;
    int slot = currentSaveSlot >= 0 ? currentSaveSlot : latestSaveSlot();
    if (slot < 0 || !loadSaveSlot(slot)) return;
    audio.playClick();
    currentScreen = Screen::Game;
    showStatusBanner("Game Resumed");
    audio.startBGM();
}

void GameFrontend::refreshSaveSlots() {
    savedGameAvailable = false;
    for (int i = 0; i < static_cast<int>(saveSlots.size()); ++i) {
        saveSlots[static_cast<std::size_t>(i)] = SaveSlotInfo{};
        saveSlots[static_cast<std::size_t>(i)].slot = i;

        std::ifstream in(savePath(i));
        if (!in) continue;

        SaveSlotInfo info;
        info.occupied = true;
        info.slot = i;
        std::string line;
        while (std::getline(in, line)) {
            std::size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            if (key == "name") info.name = value;
            else if (key == "timestamp") info.timestamp = value;
            else if (key == "level") info.level = std::atoi(value.c_str());
            else if (key == "unlockedLevel") info.unlockedLevel = std::atoi(value.c_str());
            else if (key == "waveIndex") info.waveIndex = std::atoi(value.c_str());
            else if (key == "phase") info.phase = intToPhase(std::atoi(value.c_str()));
        }
        if (info.name.empty()) info.name = "Save " + std::to_string(i + 1);
        saveSlots[static_cast<std::size_t>(i)] = info;
        savedGameAvailable = true;
    }
}

int GameFrontend::latestSaveSlot() const {
    int latest = -1;
    std::string latestStamp;
    for (const SaveSlotInfo& slot : saveSlots) {
        if (!slot.occupied) continue;
        if (latest < 0 || slot.timestamp > latestStamp) {
            latest = slot.slot;
            latestStamp = slot.timestamp;
        }
    }
    return latest;
}

int GameFrontend::firstEmptySaveSlot() const {
    for (const SaveSlotInfo& slot : saveSlots) {
        if (!slot.occupied) return slot.slot;
    }
    return -1;
}

bool GameFrontend::writeCurrentSave() {
    if (currentSaveSlot < 0 || currentSaveSlot >= static_cast<int>(saveSlots.size())) return false;
    GameSnapshot snapshot = engine.getSnapshot();
    if (snapshot.phase == GamePhase::PreGame ||
        snapshot.phase == GamePhase::GameOver) {
        return false;
    }
    if (snapshot.thresholdAcademic <= 0 || snapshot.thresholdPhysical <= 0 ||
        snapshot.thresholdMental <= 0 || snapshot.thresholdConnection <= 0) {
        return false;
    }

    std::filesystem::create_directories(saveDir());
    std::ofstream out(savePath(currentSaveSlot), std::ios::trunc);
    if (!out) return false;

    SaveSlotInfo& slot = saveSlots[static_cast<std::size_t>(currentSaveSlot)];
    if (slot.name.empty()) slot.name = "Save " + std::to_string(currentSaveSlot + 1);
    slot.timestamp = nowTimestamp();
    slot.level = currentLevel;
    slot.unlockedLevel = unlockedLevel;
    SavedEngineState state = engine.captureSaveState();
    slot.waveIndex = state.waveIndex;
    slot.phase = state.phase;
    slot.occupied = true;

    out << "version=2\n";
    out << "name=" << sanitizeSaveName(slot.name) << "\n";
    out << "timestamp=" << slot.timestamp << "\n";
    out << "level=" << currentLevel << "\n";
    out << "unlockedLevel=" << unlockedLevel << "\n";
    out << "currentQuestion=" << currentQuestion << "\n";
    out << "answers=" << joinAnswers(answers) << "\n";
    out << "selectedTower=" << towerKindToInt(selectedTowerKind) << "\n";
    out << "bilibiliDirIndex=" << bilibiliDirIndex << "\n";
    out << "phase=" << phaseToInt(state.phase) << "\n";
    out << "gold=" << state.gold << "\n";
    out << "baseHp=" << state.baseHp << "\n";
    out << "waveIndex=" << state.waveIndex << "\n";
    out << "exerciseMode=" << (state.exerciseMode ? 1 : 0) << "\n";
    out << "timeScale=" << state.timeScale << "\n";
    out << "currentAcademic=" << state.currentAcademic << "\n";
    out << "currentPhysical=" << state.currentPhysical << "\n";
    out << "currentMental=" << state.currentMental << "\n";
    out << "currentConnection=" << state.currentConnection << "\n";
    out << "thresholdAcademic=" << state.thresholdAcademic << "\n";
    out << "thresholdPhysical=" << state.thresholdPhysical << "\n";
    out << "thresholdMental=" << state.thresholdMental << "\n";
    out << "thresholdConnection=" << state.thresholdConnection << "\n";
    out << "tags=" << joinStrings(state.astiTags, '|') << "\n";
    out << "stageScoreCount=" << stageScores.size() << "\n";
    for (std::size_t i = 0; i < stageScores.size(); ++i) {
        out << "stageScore" << i << "=" << serializeStageScore(stageScores[i]) << "\n";
    }
    out << "towerCount=" << state.towers.size() << "\n";
    for (std::size_t i = 0; i < state.towers.size(); ++i) {
        const SavedTowerState& tower = state.towers[i];
        out << "tower" << i << "="
            << towerKindToInt(tower.kind) << ","
            << tower.position.x << ","
            << tower.position.y << ","
            << tower.fireDirection.x << ","
            << tower.fireDirection.y << ","
            << tower.aiLevel << ","
            << tower.cooldown << "\n";
    }
    out.flush();
    refreshSaveSlots();
    return true;
}

bool GameFrontend::loadSaveSlot(int slot) {
    if (slot < 0 || slot >= static_cast<int>(saveSlots.size())) return false;
    std::ifstream in(savePath(slot));
    if (!in) return false;

    std::vector<std::pair<std::string, std::string>> entries;
    std::string line;
    while (std::getline(in, line)) {
        std::size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        entries.emplace_back(line.substr(0, eq), line.substr(eq + 1));
    }

    auto valueOf = [&](const std::string& key, const std::string& fallback = "") {
        for (const auto& entry : entries) {
            if (entry.first == key) return entry.second;
        }
        return fallback;
    };

    currentSaveSlot = slot;
    currentLevel = std::max(1, std::min(maxLevelCount(), std::atoi(valueOf("level", "1").c_str())));
    unlockedLevel = std::max(1, std::min(maxLevelCount(), std::atoi(valueOf("unlockedLevel", "1").c_str())));
    currentQuestion = std::atoi(valueOf("currentQuestion", "0").c_str());
    answers = parseAnswers(valueOf("answers"));
    questionnaire = buildAstiQuestionnaire();
    if (answers.size() != questionnaire.getQuestions().size()) {
        answers.assign(questionnaire.getQuestions().size(), 0);
    }
    astiResult = questionnaire.score(answers);
    selectedTowerKind = intToTowerKind(std::atoi(valueOf("selectedTower", "0").c_str()));
    bilibiliDirIndex = std::atoi(valueOf("bilibiliDirIndex", "0").c_str()) % 4;
    if (bilibiliDirIndex < 0) bilibiliDirIndex = 0;
    const Vector2D dirs[] = {{1.0f, 0.0f}, {0.0f, -1.0f}, {-1.0f, 0.0f}, {0.0f, 1.0f}};
    bilibiliDir = dirs[bilibiliDirIndex];

    loadLevelDefinition(currentLevel);

    SavedEngineState state;
    state.phase = intToPhase(std::atoi(valueOf("phase", "1").c_str()));
    state.gold = std::atoi(valueOf("gold", "100").c_str());
    state.baseHp = std::atoi(valueOf("baseHp", "100").c_str());
    state.waveIndex = std::atoi(valueOf("waveIndex", "-1").c_str());
    state.exerciseMode = std::atoi(valueOf("exerciseMode", "0").c_str()) != 0;
    state.timeScale = static_cast<float>(std::atof(valueOf("timeScale", "1.0").c_str()));
    state.currentAcademic = std::atoi(valueOf("currentAcademic", "75").c_str());
    state.currentPhysical = std::atoi(valueOf("currentPhysical", "75").c_str());
    state.currentMental = std::atoi(valueOf("currentMental", "75").c_str());
    state.currentConnection = std::atoi(valueOf("currentConnection", "75").c_str());
    state.thresholdAcademic = std::atoi(valueOf("thresholdAcademic", "50").c_str());
    state.thresholdPhysical = std::atoi(valueOf("thresholdPhysical", "50").c_str());
    state.thresholdMental = std::atoi(valueOf("thresholdMental", "50").c_str());
    state.thresholdConnection = std::atoi(valueOf("thresholdConnection", "50").c_str());
    state.astiTags = splitString(valueOf("tags"), '|');
    stageScores.clear();
    const int scoreCount = std::max(0, std::atoi(valueOf("stageScoreCount", "0").c_str()));
    for (int i = 0; i < scoreCount; ++i) {
        StageScoreRecord record;
        if (parseStageScore(valueOf("stageScore" + std::to_string(i)), record)) {
            stageScores.push_back(record);
        }
    }
    std::sort(stageScores.begin(), stageScores.end(),
              [](const StageScoreRecord& a, const StageScoreRecord& b) {
                  return a.level < b.level;
              });
    stageScoreRecordedForCurrentLevel = false;

    bool repairedSave = false;
    bool completedCurrentLevelScore = false;
    for (const StageScoreRecord& record : stageScores) {
        if (record.level == currentLevel) {
            completedCurrentLevelScore = true;
            break;
        }
    }
    if (state.thresholdAcademic <= 0 || state.thresholdPhysical <= 0 ||
        state.thresholdMental <= 0 || state.thresholdConnection <= 0) {
        repairedSave = true;
        state.thresholdAcademic = astiResult.thresholdAcademic;
        state.thresholdPhysical = astiResult.thresholdPhysical;
        state.thresholdMental = astiResult.thresholdMental;
        state.thresholdConnection = astiResult.thresholdConnection;
        state.currentAcademic = std::min(100, state.thresholdAcademic + 25);
        state.currentPhysical = std::min(100, state.thresholdPhysical + 25);
        state.currentMental = std::min(100, state.thresholdMental + 25);
        state.currentConnection = std::min(100, state.thresholdConnection + 25);
        state.astiTags = astiResult.tags;
    }
    if (state.phase == GamePhase::PreGame) {
        repairedSave = true;
        state.phase = state.waveIndex >= 0 ? GamePhase::WaveCleared : GamePhase::Build;
    }
    if (currentLevel >= maxLevelCount() && completedCurrentLevelScore
        && state.phase == GamePhase::Build && state.waveIndex < 0) {
        repairedSave = true;
        state.phase = GamePhase::Victory;
        state.waveIndex = static_cast<int>(getLevelDefinition(currentLevel).waves.size()) - 1;
    }

    const int towerCount = std::max(0, std::atoi(valueOf("towerCount", "0").c_str()));
    for (int i = 0; i < towerCount; ++i) {
        std::vector<std::string> parts = splitString(valueOf("tower" + std::to_string(i)), ',');
        if (parts.size() < 7) continue;
        SavedTowerState tower;
        tower.kind = intToTowerKind(std::atoi(parts[0].c_str()));
        tower.position = {static_cast<float>(std::atof(parts[1].c_str())),
                          static_cast<float>(std::atof(parts[2].c_str()))};
        tower.fireDirection = {static_cast<float>(std::atof(parts[3].c_str())),
                               static_cast<float>(std::atof(parts[4].c_str()))};
        tower.aiLevel = std::atoi(parts[5].c_str());
        tower.cooldown = static_cast<float>(std::atof(parts[6].c_str()));
        state.towers.push_back(tower);
    }

    std::vector<SavedTowerState> validTowers;
    validTowers.reserve(state.towers.size());
    for (const SavedTowerState& tower : state.towers) {
        int row = -1;
        int col = -1;
        if (!block.worldToGrid(tower.position.x, tower.position.y, row, col)
            || !block.canPlaceTower(row, col)
            || !block.placeTowerAt(row, col)) {
            repairedSave = true;
            continue;
        }
        validTowers.push_back(tower);
    }
    state.towers = std::move(validTowers);

    engine.restoreSaveState(state);
    chestEffectMessage.clear();
    chestEffectDisplayTimer = 0.0f;
    effectManager.clear();
    selectedTowerIndex = -1;
    showExerciseGuide = false;
    uiScrollOffset = 0.0f;
    currentScreen = (engine.getPhase() == GamePhase::Victory) ? Screen::Victory : Screen::Game;
    graduationVideoPlayedForCurrentLevel = (engine.getPhase() == GamePhase::Victory);
    savedGameAvailable = true;
    if (repairedSave) {
        writeCurrentSave();
    }
    return true;
}

void GameFrontend::beginNewSaveNaming(int overwriteSlot) {
    pendingOverwriteSlot = overwriteSlot;
    int slot = overwriteSlot >= 0 ? overwriteSlot : firstEmptySaveSlot();
    if (slot < 0) slot = 0;
    saveNameInput = saveSlots[static_cast<std::size_t>(slot)].occupied
        ? saveSlots[static_cast<std::size_t>(slot)].name
        : "Save " + std::to_string(slot + 1);
    currentScreen = Screen::SaveName;
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
    chestEffectMessage.clear();
    chestEffectDisplayTimer = 0.0f;
    effectManager.clear();
    selectedTowerIndex = -1;
    showExerciseGuide = false;
    hoveredRow = -1;
    hoveredCol = -1;
    gameOverMenuSelection = 0;
    victoryMenuSelection = 0;
    stageScoreRecordedForCurrentLevel = false;
    graduationVideoPlayedForCurrentLevel = false;
    currentScreen = Screen::Game;
    savedGameAvailable = true;
    writeCurrentSave();
    const char* levelNames[] = {"Freshman Year", "Sophomore Year", "Junior Year", "Senior Year"};
    int idx = std::max(1, std::min(level, 4)) - 1;
    showStatusBanner(std::string("Level ") + std::to_string(level) + ": " + levelNames[idx]);
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
    InitWindow(kFallbackWindowWidth, kFallbackWindowHeight, "GPA Defender");
    fitWindowToCurrentMonitor();
    initVirtualCanvas();

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
        "Calculus Research Project Busy Friends Morning Class Midterm Boss Group Project Short Video Loop Unclear Syllabus Peer Pressure"
        "ASTI type result unknown Academic Grinder Balanced Survivor Fitness Defender Social Anchor"
        "Final threshold score Academic Physical Mental Connection click select continue start game retry main menu"
        "Tower guide Coffee AI Library Class Bilibili Exercise GPA recovery every five seconds wave cleared victory game over";

    int cpCount = 0;
    int* codepoints = LoadCodepoints(preloadText.c_str(), &cpCount);

    uiFont = loadUiFont(codepoints, cpCount);
    UnloadCodepoints(codepoints);
    if (uiFont.texture.id != GetFontDefault().texture.id) {
        GenTextureMipmaps(&uiFont.texture);
        SetTextureFilter(uiFont.texture, TEXTURE_FILTER_TRILINEAR);
    }
    setUiFont(uiFont);

    textureManager.loadAll();
    refreshSaveSlots();

    loadLevelDefinition(currentLevel);
    answers.resize(questionnaire.getQuestions().size(), -1);

    while (!WindowShouldClose()) {
        audio.update();
        updateMouseClickBlock();
        switch (currentScreen) {
        case Screen::MainMenu:
            runMainMenu();
            break;
        case Screen::SaveName:
            runSaveName();
            break;
        case Screen::SaveSlots:
            runSaveSlots();
            break;
        case Screen::OpeningVideo:
            runOpeningVideo();
            break;
        case Screen::Questionnaire:
            runQuestionnaire();
            break;
        case Screen::AstiSummary:
            runAstiSummary();
            break;
        case Screen::TowerIntroVideo:
            runTowerIntroVideo();
            break;
        case Screen::LevelSelect:
            runLevelSelect();
            break;
        case Screen::Game:
            runGame();
            break;
        case Screen::GraduationVideo:
            runGame();
            break;
        case Screen::GameOver:
        case Screen::Victory:
            runGame();  // render game state underneath + overlay
            break;
        }
    }

    writeCurrentSave();
    textureManager.unloadAll();
    openingVideo.unload();
    towerIntroVideo.unload();
    graduationVideo.unload();
    shutdownVirtualCanvas();
    if (uiFont.texture.id != GetFontDefault().texture.id) {
        UnloadFont(uiFont);
    }
    audio.shutdown();
    CloseWindow();
}

void GameFrontend::runMainMenu() {
    refreshSaveSlots();
    const Vector2 mouse = GetMousePosition();
    const bool clickedContinue = savedGameAvailable && primaryClickPressed() &&
        CheckCollisionPointRec(mouse, mainMenuContinueRect());
    const bool clickedNewGame = primaryClickPressed() &&
        CheckCollisionPointRec(mouse, mainMenuNewGameRect(savedGameAvailable));
    const bool clickedSaveSlots = savedGameAvailable && primaryClickPressed() &&
        CheckCollisionPointRec(mouse, mainMenuSaveSlotsRect());

    if ((savedGameAvailable && IsKeyPressed(KEY_ENTER)) || clickedContinue) {
        if (clickedContinue) blockMouseClickUntilRelease();
        continueSavedGame();
        return;
    }

    if ((!savedGameAvailable && IsKeyPressed(KEY_ENTER)) || clickedNewGame) {
        if (clickedNewGame) blockMouseClickUntilRelease();
        int empty = firstEmptySaveSlot();
        if (empty >= 0) {
            beginNewSaveNaming(empty);
        } else {
            saveSlotsForNewGame = true;
            saveSlotSelection = 0;
            currentScreen = Screen::SaveSlots;
        }
        return;
    }

    if (clickedSaveSlots) {
        blockMouseClickUntilRelease();
        saveSlotsForNewGame = false;
        saveSlotSelection = std::max(0, latestSaveSlot());
        currentScreen = Screen::SaveSlots;
        return;
    }

    beginVirtualDrawing();
    ClearBackground(Color{20, 20, 35, 255});
    drawMainMenu(&textureManager, savedGameAvailable);
    endVirtualDrawing();
}

void GameFrontend::drawSaveNameScreen() {
    ClearBackground(Color{246, 248, 244, 255});
    const char* title = "Name Save";
    int tw = measureTextF(title, 68);
    drawTextF(title, SCREEN_WIDTH / 2 - tw / 2, 215, 68, Color{35, 48, 43, 255});

    int slot = pendingOverwriteSlot >= 0 ? pendingOverwriteSlot : firstEmptySaveSlot();
    if (slot < 0) slot = 0;
    std::string subtitle = "Slot " + std::to_string(slot + 1) + " will be used.";
    tw = measureTextF(subtitle.c_str(), 30);
    drawTextF(subtitle.c_str(), SCREEN_WIDTH / 2 - tw / 2, 305, 30, Color{95, 112, 105, 255});

    Rectangle input{SCREEN_WIDTH / 2.0f - 360.0f, 420.0f, 720.0f, 80.0f};
    DrawRectangleRounded(input, 0.18f, 8, WHITE);
    DrawRectangleRoundedLines(input, 0.18f, 8, 1.4f, Color{42, 116, 86, 255});
    std::string shown = saveNameInput + ((static_cast<int>(GetTime() * 2.0) % 2 == 0) ? "|" : "");
    drawTextF(shown.c_str(), static_cast<int>(input.x + 24), static_cast<int>(input.y + 20), 34,
              Color{35, 48, 43, 255});

    Rectangle createBtn{SCREEN_WIDTH / 2.0f - 250.0f, 570.0f, 500.0f, 66.0f};
    bool createHover = CheckCollisionPointRec(GetMousePosition(), createBtn);
    DrawRectangleRounded(createBtn, 0.22f, 8, createHover ? Color{52, 132, 96, 255} : Color{42, 116, 86, 255});
    const char* create = "Create Save";
    tw = measureTextF(create, 32);
    drawTextF(create, static_cast<int>(createBtn.x + createBtn.width / 2 - tw / 2),
              static_cast<int>(createBtn.y + 16), 32, WHITE);

    Rectangle cancelBtn{SCREEN_WIDTH / 2.0f - 250.0f, 660.0f, 500.0f, 58.0f};
    bool cancelHover = CheckCollisionPointRec(GetMousePosition(), cancelBtn);
    DrawRectangleRounded(cancelBtn, 0.22f, 8, cancelHover ? Color{244, 238, 235, 255} : WHITE);
    DrawRectangleRoundedLines(cancelBtn, 0.22f, 8, 1.0f, cancelHover ? Color{190, 95, 70, 255} : Color{210, 218, 210, 255});
    const char* cancel = "Cancel";
    tw = measureTextF(cancel, 28);
    drawTextF(cancel, static_cast<int>(cancelBtn.x + cancelBtn.width / 2 - tw / 2),
              static_cast<int>(cancelBtn.y + 14), 28, cancelHover ? Color{170, 72, 55, 255} : Color{35, 48, 43, 255});
}

void GameFrontend::runSaveName() {
    int ch = GetCharPressed();
    while (ch > 0) {
        if (ch >= 32 && ch <= 126 && saveNameInput.size() < 32) {
            saveNameInput.push_back(static_cast<char>(ch));
            saveNameInput = sanitizeSaveName(saveNameInput);
        }
        ch = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && !saveNameInput.empty()) {
        saveNameInput.pop_back();
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        currentScreen = Screen::MainMenu;
        pendingOverwriteSlot = -1;
    }

    const Vector2 mouse = GetMousePosition();
    Rectangle createBtn{SCREEN_WIDTH / 2.0f - 250.0f, 570.0f, 500.0f, 66.0f};
    Rectangle cancelBtn{SCREEN_WIDTH / 2.0f - 250.0f, 660.0f, 500.0f, 58.0f};
    bool create = IsKeyPressed(KEY_ENTER) ||
        (primaryClickPressed() && CheckCollisionPointRec(mouse, createBtn));
    bool cancel = primaryClickPressed() && CheckCollisionPointRec(mouse, cancelBtn);

    if (create) {
        blockMouseClickUntilRelease();
        saveNameInput = sanitizeSaveName(saveNameInput);
        startNewGameFlow();
        return;
    }
    if (cancel) {
        blockMouseClickUntilRelease();
        pendingOverwriteSlot = -1;
        currentScreen = Screen::MainMenu;
        return;
    }

    beginVirtualDrawing();
    drawSaveNameScreen();
    endVirtualDrawing();
}

void GameFrontend::drawSaveSlotsScreen() {
    ClearBackground(Color{246, 248, 244, 255});
    const char* title = saveSlotsForNewGame ? "Choose Slot to Overwrite" : "Save Slots";
    int tw = measureTextF(title, 62);
    drawTextF(title, SCREEN_WIDTH / 2 - tw / 2, 105, 62, Color{35, 48, 43, 255});

    const float x = SCREEN_WIDTH / 2.0f - 430.0f;
    float y = 220.0f;
    for (int i = 0; i < 5; ++i) {
        Rectangle rect{x, y + i * 118.0f, 860.0f, 92.0f};
        bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
        bool selected = saveSlotSelection == i;
        DrawRectangleRounded(rect, 0.16f, 8,
                             selected ? Color{219, 239, 228, 255} :
                             (hover ? Color{238, 244, 240, 255} : WHITE));
        DrawRectangleRoundedLines(rect, 0.16f, 8, 1.2f,
                                  selected || hover ? Color{42, 116, 86, 255} : Color{210, 218, 210, 255});

        const SaveSlotInfo& slot = saveSlots[static_cast<std::size_t>(i)];
        std::string name = slot.occupied ? slot.name : "Empty Slot";
        std::string left = "Slot " + std::to_string(i + 1) + "  " + name;
        drawTextF(left.c_str(), static_cast<int>(rect.x + 24), static_cast<int>(rect.y + 15), 32,
                  Color{35, 48, 43, 255});
        std::string detail = slot.occupied
            ? ("Level " + std::to_string(slot.level) + "  Wave " + std::to_string(slot.waveIndex + 1) +
               "  " + slot.timestamp)
            : (saveSlotsForNewGame ? "Select to create a new save here." : "Available for a new game.");
        drawTextF(detail.c_str(), static_cast<int>(rect.x + 24), static_cast<int>(rect.y + 54), 24,
                  Color{95, 112, 105, 255});
    }

    const char* hint = saveSlotsForNewGame
        ? "Click a slot or press ENTER to overwrite it. ESC returns to menu."
        : "Click a save to load it. Click an empty slot to start a named new game.";
    tw = measureTextF(hint, 26);
    drawTextF(hint, SCREEN_WIDTH / 2 - tw / 2, 845, 26, Color{95, 112, 105, 255});

    Rectangle backBtn{SCREEN_WIDTH / 2.0f - 180.0f, 905.0f, 360.0f, 58.0f};
    bool backHover = CheckCollisionPointRec(GetMousePosition(), backBtn);
    DrawRectangleRounded(backBtn, 0.22f, 8, backHover ? Color{244, 238, 235, 255} : WHITE);
    DrawRectangleRoundedLines(backBtn, 0.22f, 8, 1.0f, backHover ? Color{190, 95, 70, 255} : Color{210, 218, 210, 255});
    const char* back = "Back";
    tw = measureTextF(back, 28);
    drawTextF(back, static_cast<int>(backBtn.x + backBtn.width / 2 - tw / 2),
              static_cast<int>(backBtn.y + 14), 28, backHover ? Color{170, 72, 55, 255} : Color{35, 48, 43, 255});
}

void GameFrontend::runSaveSlots() {
    refreshSaveSlots();
    if (IsKeyPressed(KEY_ESCAPE)) {
        currentScreen = Screen::MainMenu;
        return;
    }
    if (IsKeyPressed(KEY_UP)) saveSlotSelection = std::max(0, saveSlotSelection - 1);
    if (IsKeyPressed(KEY_DOWN)) saveSlotSelection = std::min(4, saveSlotSelection + 1);

    const Vector2 mouse = GetMousePosition();
    const float x = SCREEN_WIDTH / 2.0f - 430.0f;
    float y = 220.0f;
    int clickedSlot = -1;
    if (primaryClickPressed()) {
        for (int i = 0; i < 5; ++i) {
            Rectangle rect{x, y + i * 118.0f, 860.0f, 92.0f};
            if (CheckCollisionPointRec(mouse, rect)) {
                clickedSlot = i;
                saveSlotSelection = i;
            }
        }
        Rectangle backBtn{SCREEN_WIDTH / 2.0f - 180.0f, 905.0f, 360.0f, 58.0f};
        if (CheckCollisionPointRec(mouse, backBtn)) {
            blockMouseClickUntilRelease();
            currentScreen = Screen::MainMenu;
            return;
        }
    }

    bool activate = IsKeyPressed(KEY_ENTER) || clickedSlot >= 0;
    if (activate) {
        blockMouseClickUntilRelease();
        int slot = clickedSlot >= 0 ? clickedSlot : saveSlotSelection;
        if (saveSlotsForNewGame || !saveSlots[static_cast<std::size_t>(slot)].occupied) {
            beginNewSaveNaming(slot);
            saveSlotsForNewGame = false;
            return;
        }
        if (loadSaveSlot(slot)) {
            audio.playClick();
            audio.startBGM();
            showStatusBanner("Save Loaded");
            return;
        }
    }

    beginVirtualDrawing();
    drawSaveSlotsScreen();
    endVirtualDrawing();
}

void GameFrontend::runQuestionnaire() {
    const auto& questions = questionnaire.getQuestions();

    auto chooseAnswer = [&](int choice) {
        if (choice < 0 ||
            choice >= static_cast<int>(questions[currentQuestion].options.size())) {
            return false;
        }
        audio.playClick();
        answers[currentQuestion] = choice;
        if (currentQuestion < static_cast<int>(questions.size()) - 1) {
            ++currentQuestion;
        } else {
            astiResult = questionnaire.score(answers);
            currentScreen = Screen::AstiSummary;
        }
        return true;
    };

    for (int key = KEY_ONE; key <= KEY_FOUR; ++key) {
        if (IsKeyPressed(key)) {
            int choice = key - KEY_ONE;
            if (chooseAnswer(choice)) return;
        }
    }

    if (primaryClickPressed()) {
        Vector2 mouse = GetMousePosition();
        for (std::size_t i = 0; i < questions[currentQuestion].options.size(); ++i) {
            if (CheckCollisionPointRec(mouse, questionnaireOptionRect(i))) {
                blockMouseClickUntilRelease();
                if (chooseAnswer(static_cast<int>(i))) return;
            }
        }
    }

    if (IsKeyPressed(KEY_BACKSPACE) && currentQuestion > 0) {
        --currentQuestion;
    }

    beginVirtualDrawing();
    ClearBackground(Color{20, 20, 35, 255});
    drawQuestionnaire(questionnaire, currentQuestion, answers);
    endVirtualDrawing();
}

void GameFrontend::runAstiSummary() {
    const bool clickedContinue = primaryClickPressed() &&
        CheckCollisionPointRec(GetMousePosition(), astiContinueRect());

    if (IsKeyPressed(KEY_ENTER) || clickedContinue) {
        audio.playClick();
        if (clickedContinue) blockMouseClickUntilRelease();
        currentLevel = 1;
        unlockedLevel = 1;
        selectedTowerIndex = -1;
        showExerciseGuide = false;
        startTowerIntroVideo();
        return;
    }

    beginVirtualDrawing();
    ClearBackground(Color{20, 20, 35, 255});
    drawAstiSummary(astiResult);
    endVirtualDrawing();
}

void GameFrontend::runLevelSelect() {
    const int cardW = 309;
    const int cardH = 500;
    const int gap = 70;
    const int totalW = cardW * 4 + gap * 3;
    const int startX = SCREEN_WIDTH / 2 - totalW / 2;
    const int cardY = 280;
    const Rectangle retryRect = levelSelectRetakeRect();
    const Rectangle returnRect = levelSelectReturnRect();

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
            if (primaryClickPressed() && hoveredLevel <= unlockedLevel) {
                audio.playClick();
                blockMouseClickUntilRelease();
                startLevel(hoveredLevel);
                return;
            }
        }
    }

    if (CheckCollisionPointRec(mouse, retryRect) && primaryClickPressed()) {
        audio.playClick();
        blockMouseClickUntilRelease();
        questionnaire = buildAstiQuestionnaire();
        answers.assign(questionnaire.getQuestions().size(), -1);
        currentQuestion = 0;
        currentLevel = 1;
        unlockedLevel = 1;
        selectedTowerIndex = -1;
        showExerciseGuide = false;
        engine = GameEngine();
        currentScreen = Screen::Questionnaire;
        return;
    }

    if (CheckCollisionPointRec(mouse, returnRect) && primaryClickPressed()) {
        audio.playClick();
        blockMouseClickUntilRelease();
        selectedTowerIndex = -1;
        showExerciseGuide = false;
        statusBannerTimer = 0.0f;
        statusBannerText.clear();
        audio.stopBGM();
        currentScreen = Screen::MainMenu;
        return;
    }

    beginVirtualDrawing();
    ClearBackground(Color{20, 20, 35, 255});
    drawLevelSelect(unlockedLevel, hoveredLevel, &textureManager);
    endVirtualDrawing();
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

    if (primaryClickPressed() && mouse.x < UI_PANEL_X && mouse.y < MAP_OFFSET_Y) {
        TowerKind kinds[] = {TowerKind::Coffee, TowerKind::AI, TowerKind::Library,
                             TowerKind::Class, TowerKind::Bilibili};
        for (int i = 0; i < 5; ++i) {
            if (CheckCollisionPointRec(mouse, seedBarTowerRect(i))) {
                audio.playClick();
                selectedTowerKind = kinds[i];
                selectedTowerIndex = -1;
                showExerciseGuide = false;
                blockMouseClickUntilRelease();
                return;
            }
        }
        blockMouseClickUntilRelease();
        return;
    }

    if (mouse.x > UI_PANEL_X) {
        const float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            uiScrollOffset -= wheel * 60.0f;
            if (uiScrollOffset < 0.0f) uiScrollOffset = 0.0f;
            if (uiScrollOffset > 0.0f) uiScrollOffset = 0.0f;
        }
    }

    // UI panel clicks
    if (mouse.x > UI_PANEL_X) {
        int lx = UI_PANEL_X + 20;
        int w = UI_PANEL_WIDTH - 39;

        // Y positions must match drawUI layout
        int yExModeStart = 457;
        int ySpeedStart = 521;
        int yStartWaveStart = 585;
        int yReturnStart = 669;
        int yLevelSelectStart = 735;

        if (primaryClickPressed()) {
            // Exercise mode toggle
            Rectangle exBtn = {static_cast<float>(lx),
                               static_cast<float>(yExModeStart) - uiScrollOffset,
                               static_cast<float>(w), 54.0f};
            if (CheckCollisionPointRec(mouse, exBtn)) {
                audio.playClick();
                engine.setExerciseMode(!engine.getExerciseMode());
                showExerciseGuide = true;
            }

            Rectangle slowBtn = {static_cast<float>(lx),
                                 static_cast<float>(ySpeedStart) - uiScrollOffset,
                                 165.0f, 54.0f};
            Rectangle normalBtn = {static_cast<float>(lx + 178),
                                   static_cast<float>(ySpeedStart) - uiScrollOffset,
                                   120.0f, 54.0f};
            Rectangle fastBtn = {static_cast<float>(lx + 311),
                                 static_cast<float>(ySpeedStart) - uiScrollOffset,
                                 165.0f, 54.0f};
            if (CheckCollisionPointRec(mouse, slowBtn)) {
                audio.playClick();
                engine.setTimeScale(engine.getTimeScale() - 0.5f);
            }
            if (CheckCollisionPointRec(mouse, normalBtn)) {
                audio.playClick();
                engine.setTimeScale(1.0f);
            }
            if (CheckCollisionPointRec(mouse, fastBtn)) {
                audio.playClick();
                engine.setTimeScale(engine.getTimeScale() + 0.5f);
            }

            // Start Wave button
            Rectangle swBtn = {static_cast<float>(lx),
                               static_cast<float>(yStartWaveStart) - uiScrollOffset,
                               static_cast<float>(w), 72.0f};
            if (CheckCollisionPointRec(mouse, swBtn)) {
                audio.playClick();
                if (engine.startWave()) {
                    GameSnapshot snap = engine.getSnapshot();
                    showStatusBanner("Wave " + std::to_string(snap.waveIndex + 1) + " Started");
                }
            }

            Rectangle returnBtn = {static_cast<float>(lx),
                                   static_cast<float>(yReturnStart) - uiScrollOffset,
                                   static_cast<float>(w), 54.0f};
            if (CheckCollisionPointRec(mouse, returnBtn)) {
                audio.playClick();
                blockMouseClickUntilRelease();
                returnToMainMenuWithSave();
                return;
            }

            Rectangle levelBtn = {static_cast<float>(lx),
                                  static_cast<float>(yLevelSelectStart) - uiScrollOffset,
                                  static_cast<float>(w), 54.0f};
            if (CheckCollisionPointRec(mouse, levelBtn)) {
                audio.playClick();
                writeCurrentSave();
                refreshSaveSlots();
                selectedTowerIndex = -1;
                showExerciseGuide = false;
                statusBannerTimer = 0.0f;
                statusBannerText.clear();
                audio.stopBGM();
                blockMouseClickUntilRelease();
                currentScreen = Screen::LevelSelect;
                return;
            }
        }

        // Click on UI panel = don't interact with map
        return;
    }

    // Map interaction
    if (primaryClickPressed()) {
        Vector2D mouseWorld{mouse.x - MAP_OFFSET_X, mouse.y - MAP_OFFSET_Y};
        if (engine.tryArmChest(mouseWorld, 46.0f)) {
            audio.playClick();
            blockMouseClickUntilRelease();
            return;
        }

        if (selectedTowerIndex >= 0 &&
            selectedTowerIndex < static_cast<int>(engine.getTowers().size())) {
            const auto& selectedTower = engine.getTowers()[static_cast<std::size_t>(selectedTowerIndex)];
            const AITower* aiTower = dynamic_cast<const AITower*>(selectedTower.get());
            if (aiTower != nullptr && aiTower->nextUpgradeCost() >= 0) {
                Vector2D pos = selectedTower->getPosition();
                Rectangle upgradeBtn{
                    MAP_OFFSET_X + pos.x + 34.0f,
                    MAP_OFFSET_Y + pos.y - 82.0f,
                    150.0f,
                    46.0f
                };
                if (CheckCollisionPointRec(mouse, upgradeBtn)) {
                    audio.playClick();
                    blockMouseClickUntilRelease();
                    engine.tryUpgradeAiTower(static_cast<std::size_t>(selectedTowerIndex));
                    return;
                }
            }
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
                blockMouseClickUntilRelease();
                selectedTowerIndex = clickedTower;
                showExerciseGuide = false;
            } else {
                blockMouseClickUntilRelease();
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
    handleBuildInput();
}

void GameFrontend::handleGlobalInput() {
    if (IsKeyPressed(KEY_SPACE)) {
        GamePhase phase = engine.getPhase();
        if (phase == GamePhase::Build || phase == GamePhase::WaveCleared) {
            if (engine.startWave()) {
                GameSnapshot snap = engine.getSnapshot();
                showStatusBanner("Wave " + std::to_string(snap.waveIndex + 1) + " Started");
            }
        }
    }
    if (IsKeyPressed(KEY_E)) {
        engine.setExerciseMode(!engine.getExerciseMode());
        showExerciseGuide = true;
    }
    if (IsKeyPressed(KEY_ZERO)) {
        engine.setTimeScale(1.0f);
    }
}

void GameFrontend::startOpeningVideo() {
    std::string videoPath = assetExists("assets/video/opening.mp4")
        ? resolveAssetPath("assets/video/opening.mp4")
        : resolveAssetPath("assets/video/开场小动画.mp4");
    if (openingVideo.isLoaded()) {
        if (openingVideo.restart()) {
            currentScreen = Screen::OpeningVideo;
            return;
        }
        openingVideo.unload();
    }
    if (openingVideo.load(videoPath)) {
        currentScreen = Screen::OpeningVideo;
        return;
    }
    currentScreen = Screen::Questionnaire;
}

void GameFrontend::updateOpeningVideo(float dt) {
    openingVideo.update(dt);
    if (openingVideo.isFinished() || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)
        || primaryClickPressed()) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) blockMouseClickUntilRelease();
        openingVideo.unload();
        currentScreen = Screen::Questionnaire;
    }
}

void GameFrontend::renderOpeningVideo() {
    beginVirtualDrawing();
    openingVideo.draw();
    endVirtualDrawing();
}

void GameFrontend::runOpeningVideo() {
    float dt = GetFrameTime();
    updateOpeningVideo(dt);
    renderOpeningVideo();
}

void GameFrontend::startTowerIntroVideo() {
    std::string videoPath = assetExists("assets/video/tower_monster_intro.mp4")
        ? resolveAssetPath("assets/video/tower_monster_intro.mp4")
        : resolveAssetPath("assets/video/塔和怪兽介绍.mp4");
    if (towerIntroVideo.isLoaded()) {
        if (towerIntroVideo.restart()) {
            currentScreen = Screen::TowerIntroVideo;
            return;
        }
        towerIntroVideo.unload();
    }
    if (towerIntroVideo.load(videoPath)) {
        currentScreen = Screen::TowerIntroVideo;
        return;
    }
    showStatusBanner("Choose a Level");
    currentScreen = Screen::LevelSelect;
}

void GameFrontend::updateTowerIntroVideo(float dt) {
    towerIntroVideo.update(dt);
    if (towerIntroVideo.isFinished() || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)
        || primaryClickPressed()) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) blockMouseClickUntilRelease();
        towerIntroVideo.unload();
        showStatusBanner("Choose a Level");
        currentScreen = Screen::LevelSelect;
    }
}

void GameFrontend::renderTowerIntroVideo() {
    beginVirtualDrawing();
    towerIntroVideo.draw();
    endVirtualDrawing();
}

void GameFrontend::runTowerIntroVideo() {
    float dt = GetFrameTime();
    updateTowerIntroVideo(dt);
    renderTowerIntroVideo();
}

void GameFrontend::startGraduationVideo() {
    graduationVideoPlayedForCurrentLevel = true;
    std::string videoPath = assetExists("assets/video/settlement.mp4")
        ? resolveAssetPath("assets/video/settlement.mp4")
        : resolveAssetPath("assets/video/结算小动画.mp4");
    if (graduationVideo.isLoaded()) {
        if (graduationVideo.restart()) {
            currentScreen = Screen::GraduationVideo;
            return;
        }
        graduationVideo.unload();
    }
    if (graduationVideo.load(videoPath)) {
        currentScreen = Screen::GraduationVideo;
        return;
    }
    currentScreen = Screen::Victory;
}

void GameFrontend::updateGraduationVideo(float dt) {
    graduationVideo.update(dt);
    if (graduationVideo.isFinished() || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)
        || primaryClickPressed()) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) blockMouseClickUntilRelease();
        graduationVideo.unload();
        currentScreen = Screen::Victory;
    }
}

void GameFrontend::renderGraduationVideo() {
    beginVirtualDrawing();
    graduationVideo.draw();
    endVirtualDrawing();
}

void GameFrontend::handleChestEvents() {
    for (const ChestEvent& event : engine.consumeChestEvents()) {
        switch (event.type) {
        case ChestEventType::Targeted:
            chestEffectMessage = "Chest targeted. Towers can attack it now.";
            break;
        case ChestEventType::MemoryActivated:
            chestEffectMessage = "Memory Surge: tower damage halved for 10 seconds!";
            break;
        case ChestEventType::HellBossSpawned:
            chestEffectMessage = "Hell Mode: an extra boss is incoming!";
            audio.playEnemyDeath();
            break;
        case ChestEventType::RewardGranted:
            chestEffectMessage = "Reward: Gold +" + std::to_string(event.amount) + "!";
            break;
        case ChestEventType::GambleWon:
            chestEffectMessage = "Gamble won: Gold +" + std::to_string(event.amount) + "!";
            break;
        case ChestEventType::GambleLost:
            chestEffectMessage = "Gamble lost: Gold -" + std::to_string(event.amount) + "!";
            break;
        }
        chestEffectDisplayTimer = 3.0f;
    }
}

void GameFrontend::updateGame(float dt) {
    if (chestEffectDisplayTimer > 0.0f) {
        chestEffectDisplayTimer -= dt;
        if (chestEffectDisplayTimer <= 0.0f) {
            chestEffectDisplayTimer = 0.0f;
            chestEffectMessage.clear();
        }
    }

    int aliveBefore = engine.getWaveManager().getActiveEnemyCount();
    GamePhase phaseBefore = engine.getPhase();

    engine.update(dt);
    handleChestEvents();
    effectManager.spawnAll(engine.consumeAttackEvents());
    effectManager.update(dt);

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
            showStatusBanner("Wave Cleared");
        } else if (phaseAfter == GamePhase::GameOver) {
            savedGameAvailable = false;
            audio.stopBGM();
            audio.playGameOver();
            showStatusBanner("Game Over");
        } else if (phaseAfter == GamePhase::Victory) {
            if (currentLevel < maxLevelCount()) {
                unlockedLevel = std::max(unlockedLevel, currentLevel + 1);
            }
            if (!stageScoreRecordedForCurrentLevel) {
                recordCurrentStageScore();
            }
            writeCurrentSave();
            savedGameAvailable = false;
            audio.stopBGM();
            audio.playVictory();
            showStatusBanner("Victory");
        }
    }

    if (phaseAfter == GamePhase::GameOver) {
        currentScreen = Screen::GameOver;
    } else if (phaseAfter == GamePhase::Victory) {
        if (statusBannerTimer > 0.0f) {
            currentScreen = Screen::Game;
        } else if (currentLevel >= maxLevelCount() && !graduationVideoPlayedForCurrentLevel) {
            startGraduationVideo();
        } else {
            currentScreen = Screen::Victory;
        }
    }
}

void GameFrontend::renderGame() {
    beginVirtualDrawing();
    ClearBackground(Color{246, 248, 244, 255});

    drawSeedBar(engine.getGold(), selectedTowerKind, &textureManager);
    drawMap(block, &textureManager);
    GameSnapshot snap = engine.getSnapshot();
    drawBaseHealth(block, snap.baseHp, snap.baseMaxHp);

    // Hover preview for tower placement
    GamePhase phase = engine.getPhase();
    bool canBuild = (phase == GamePhase::Build || phase == GamePhase::WaveRunning
        || phase == GamePhase::WaveCleared);
    if (canBuild && hoveredRow >= 0 && hoveredCol >= 0 && GetMousePosition().x < UI_PANEL_X) {
        drawHoverPreview(hoveredRow, hoveredCol, selectedTowerKind,
                         {static_cast<float>(bilibiliDir.x), static_cast<float>(bilibiliDir.y)},
                         block, &textureManager);
    }

    drawTowers(engine.getTowers(), selectedTowerIndex, &textureManager);

    if (selectedTowerIndex >= 0 &&
        selectedTowerIndex < static_cast<int>(engine.getTowers().size())) {
        const auto& selectedTower = engine.getTowers()[static_cast<std::size_t>(selectedTowerIndex)];
        const AITower* aiTower = dynamic_cast<const AITower*>(selectedTower.get());
        if (aiTower != nullptr) {
            Vector2D pos = selectedTower->getPosition();
            Rectangle upgradeBtn{
                MAP_OFFSET_X + pos.x + 34.0f,
                MAP_OFFSET_Y + pos.y - 82.0f,
                150.0f,
                46.0f
            };
            const int nextCost = aiTower->nextUpgradeCost();
            const bool canUpgrade = nextCost >= 0 && engine.getGold() >= nextCost;
            DrawRectangleRounded(upgradeBtn, 0.22f, 8,
                canUpgrade ? Color{45, 105, 65, 235} : Color{70, 70, 80, 225});
            DrawRectangleRoundedLines(upgradeBtn, 0.22f, 8, 1.0f,
                canUpgrade ? Color{160, 255, 180, 240} : Color{150, 150, 160, 220});

            std::string label = nextCost >= 0
                ? "Upgrade " + std::to_string(nextCost) + "g"
                : "Max Level";
            int tw = measureTextF(label.c_str(), 20);
            drawTextF(label.c_str(),
                      static_cast<int>(upgradeBtn.x + upgradeBtn.width / 2 - tw / 2),
                      static_cast<int>(upgradeBtn.y + 11),
                      20,
                      canUpgrade ? WHITE : LIGHTGRAY);
        }
    }

    if (phase == GamePhase::WaveRunning) {
        drawEnemies(engine.getWaveManager().getLiveEnemies(), &textureManager);
    }

    drawChests(engine.getActiveChests(), &textureManager);
    effectManager.draw();

    drawUI(snap, engine.getGold(), currentLevel, selectedTowerKind,
           engine.getExerciseMode(), selectedTowerIndex, showExerciseGuide,
           engine.getTimeScale(), uiScrollOffset, &textureManager);

    // Overlays
    if (currentScreen == Screen::GameOver) {
        drawGameOver(gameOverMenuSelection);
        Vector2 mouse = GetMousePosition();
        const int optionCount = 3;
        for (int i = 0; i < optionCount; ++i) {
            if (CheckCollisionPointRec(mouse, gameOverOptionRect(i))) {
                gameOverMenuSelection = i;
            }
        }
        if (IsKeyPressed(KEY_UP)) {
            gameOverMenuSelection = std::max(0, gameOverMenuSelection - 1);
        }
        if (IsKeyPressed(KEY_DOWN)) {
            gameOverMenuSelection = std::min(optionCount - 1, gameOverMenuSelection + 1);
        }
        const bool clickedOverlay = primaryClickPressed();
        if (IsKeyPressed(KEY_ENTER) || clickedOverlay) {
            if (clickedOverlay) {
                bool clickedOption = false;
                for (int i = 0; i < optionCount; ++i) {
                    if (CheckCollisionPointRec(mouse, gameOverOptionRect(i))) {
                        gameOverMenuSelection = i;
                        clickedOption = true;
                    }
                }
                if (!clickedOption) {
                    blockMouseClickUntilRelease();
                    endVirtualDrawing();
                    return;
                }
                blockMouseClickUntilRelease();
            }
            if (gameOverMenuSelection == 0) {
                // Retry current level
                retryCurrentLevel();
            } else if (gameOverMenuSelection == 1) {
                selectedTowerIndex = -1;
                showExerciseGuide = false;
                savedGameAvailable = false;
                engine = GameEngine();
                currentScreen = Screen::LevelSelect;
            } else {
                selectedTowerIndex = -1;
                showExerciseGuide = false;
                savedGameAvailable = false;
                engine = GameEngine();
                currentScreen = Screen::MainMenu;
            }
            gameOverMenuSelection = 0;
        }
    } else if (currentScreen == Screen::Victory) {
        bool hasNextLevel = (currentLevel < maxLevelCount());
        drawVictory(victoryMenuSelection, hasNextLevel, currentStageScore(), stageScores);
        Vector2 mouse = GetMousePosition();
        const int optionCount = hasNextLevel ? 3 : 2;
        for (int i = 0; i < optionCount; ++i) {
            if (CheckCollisionPointRec(mouse, victoryOptionRect(i, hasNextLevel))) {
                victoryMenuSelection = i;
            }
        }
        if (IsKeyPressed(KEY_UP)) {
            victoryMenuSelection = std::max(0, victoryMenuSelection - 1);
        }
        if (IsKeyPressed(KEY_DOWN)) {
            victoryMenuSelection = std::min(optionCount - 1, victoryMenuSelection + 1);
        }
        const bool clickedOverlay = primaryClickPressed();
        if (IsKeyPressed(KEY_ENTER) || clickedOverlay) {
            if (clickedOverlay) {
                bool clickedOption = false;
                for (int i = 0; i < optionCount; ++i) {
                    if (CheckCollisionPointRec(mouse, victoryOptionRect(i, hasNextLevel))) {
                        victoryMenuSelection = i;
                        clickedOption = true;
                    }
                }
                if (!clickedOption) {
                    blockMouseClickUntilRelease();
                    endVirtualDrawing();
                    return;
                }
                blockMouseClickUntilRelease();
            }
            if (victoryMenuSelection == 0 && hasNextLevel) {
                // Continue to next level
                goToNextLevel();
            } else if ((hasNextLevel && victoryMenuSelection == 1) ||
                       (!hasNextLevel && victoryMenuSelection == 0)) {
                selectedTowerIndex = -1;
                showExerciseGuide = false;
                savedGameAvailable = false;
                engine = GameEngine();
                currentScreen = Screen::LevelSelect;
            } else {
                selectedTowerIndex = -1;
                showExerciseGuide = false;
                savedGameAvailable = false;
                engine = GameEngine();
                currentScreen = Screen::MainMenu;
            }
            victoryMenuSelection = 0;
        }
    }

    // 绘制宝箱效果提示
    if (chestEffectDisplayTimer > 0.0f && !chestEffectMessage.empty()) {
        const std::string& msg = chestEffectMessage;
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

    if (statusBannerTimer > 0.0f && !statusBannerText.empty()) {
        float alpha = std::min(1.0f, statusBannerTimer / 0.35f);
        Color bg{255, 255, 252, static_cast<unsigned char>(225 * alpha)};
        Color line{42, 116, 86, static_cast<unsigned char>(230 * alpha)};
        Color text{32, 41, 38, static_cast<unsigned char>(255 * alpha)};
        int fontSize = 58;
        int tw = measureTextF(statusBannerText.c_str(), fontSize);
        Rectangle banner{
            SCREEN_WIDTH / 2.0f - std::max(360.0f, static_cast<float>(tw) / 2.0f + 54.0f),
            SCREEN_HEIGHT / 2.0f - 76.0f,
            std::max(720.0f, static_cast<float>(tw) + 108.0f),
            130.0f
        };
        DrawRectangleRounded(banner, 0.18f, 8, bg);
        DrawRectangleRoundedLines(banner, 0.18f, 8, 2.0f, line);
        drawTextF(statusBannerText.c_str(),
                  static_cast<int>(SCREEN_WIDTH / 2 - tw / 2),
                  static_cast<int>(banner.y + 34),
                  fontSize, text);
    }

    endVirtualDrawing();
}

void GameFrontend::runGame() {
    float dt = GetFrameTime();
    if (statusBannerTimer > 0.0f) {
        statusBannerTimer -= dt;
        if (statusBannerTimer < 0.0f) statusBannerTimer = 0.0f;
    }

    if (currentScreen == Screen::GraduationVideo) {
        updateGraduationVideo(dt);
        renderGraduationVideo();
        return;
    }

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
