#include "frontend/Renderer.h"

#include "gpa_defender/PlayerStats.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace frontend {

// Font storage for CJK text rendering
static Font gUiFont;

void setUiFont(Font font) { gUiFont = font; }
const Font& getUiFont() { return gUiFont; }

int measureTextF(const char* text, int fontSize) {
    Vector2 sz = MeasureTextEx(gUiFont, text, static_cast<float>(fontSize), 1.0f);
    return static_cast<int>(sz.x);
}

void drawTextF(const char* text, int x, int y, int fontSize, Color color) {
    DrawTextEx(gUiFont, text,
               {static_cast<float>(x), static_cast<float>(y)},
               static_cast<float>(fontSize), 1.0f, color);
}

std::vector<std::string> splitTextLines(const std::string& text) {
    std::vector<std::string> lines;
    std::string current;

    for (char ch : text) {
        if (ch == '\n') {
            if (!current.empty()) lines.push_back(current);
            current.clear();
            continue;
        }
        current += ch;
    }

    if (!current.empty()) lines.push_back(current);
    if (lines.empty()) lines.push_back("");
    return lines;
}

void drawTextLinesF(const std::vector<std::string>& lines, int x, int y,
                    int fontSize, int lineHeight, Color color) {
    for (size_t i = 0; i < lines.size(); ++i) {
        drawTextF(lines[i].c_str(), x, y + static_cast<int>(i) * lineHeight,
                  fontSize, color);
    }
}

Color tileColor(TileType type) {
    switch (type) {
    case TileType::Wall:     return DARKGRAY;
    case TileType::Path:     return Color{210, 180, 140, 255};
    case TileType::Highland: return Color{60, 140, 60, 255};
    case TileType::Spawn:    return Color{180, 60, 60, 255};
    case TileType::Base:     return Color{60, 60, 180, 255};
    }
    return GRAY;
}

Color towerColor(const std::string& name) {
    if (name.find("Coffee") != std::string::npos)   return Color{139, 69, 19, 255};
    if (name.find("AI") != std::string::npos)        return Color{128, 0, 128, 255};
    if (name.find("Library") != std::string::npos)   return Color{0, 0, 139, 255};
    if (name.find("Class") != std::string::npos)     return Color{255, 165, 0, 255};
    if (name.find("Bilibili") != std::string::npos)  return Color{255, 105, 180, 255};
    return GRAY;
}

Color enemyColor(const std::string& name) {
    if (name.find("期中") != std::string::npos || name.find("Midterm") != std::string::npos)
        return Color{180, 30, 30, 255};
    if (name.find("微积分") != std::string::npos || name.find("线代") != std::string::npos ||
        name.find("Calculus") != std::string::npos)
        return RED;
    if (name.find("科研") != std::string::npos || name.find("Research") != std::string::npos)
        return Color{100, 0, 100, 255};
    if (name.find("社交") != std::string::npos || name.find("Friends") != std::string::npos)
        return GOLD;
    if (name.find("早八") != std::string::npos || name.find("Morning") != std::string::npos)
        return ORANGE;
    if (name.find("小组") != std::string::npos || name.find("Group") != std::string::npos)
        return GRAY;
    if (name.find("短视频") != std::string::npos || name.find("Video") != std::string::npos)
        return MAGENTA;
    if (name.find("考纲") != std::string::npos || name.find("Syllabus") != std::string::npos)
        return LIME;
    if (name.find("同辈") != std::string::npos || name.find("压力") != std::string::npos ||
        name.find("Pressure") != std::string::npos)
        return SKYBLUE;
    return RED;
}

Color indicatorColor(int idx) {
    switch (idx) {
    case 0: return Color{70, 130, 200, 255};
    case 1: return Color{200, 80, 80, 255};
    case 2: return Color{140, 80, 180, 255};
    case 3: return Color{80, 180, 100, 255};
    }
    return WHITE;
}

Color phaseColor(GamePhase phase) {
    switch (phase) {
    case GamePhase::Build:       return Color{100, 180, 100, 255};
    case GamePhase::WaveRunning: return Color{200, 80, 80, 255};
    case GamePhase::WaveCleared: return Color{200, 180, 60, 255};
    case GamePhase::GameOver:    return Color{180, 60, 60, 255};
    case GamePhase::Victory:     return Color{80, 180, 80, 255};
    default:                     return GRAY;
    }
}

Color chestColor(ChestType type) {
    switch (type) {
    case ChestType::Memory: return Color{150, 150, 200, 255};
    case ChestType::Hell:   return Color{200, 60, 60, 255};
    case ChestType::Reward: return Color{255, 200, 50, 255};
    case ChestType::Gamble: return Color{50, 200, 100, 255};
    }
    return WHITE;
}

const char* chestLabel(ChestType type) {
    switch (type) {
    case ChestType::Memory: return "?";
    case ChestType::Hell:   return "!";
    case ChestType::Reward: return "$";
    case ChestType::Gamble: return "%";
    }
    return "";
}

Vector2 gridToScreen(int row, int col) {
    return {static_cast<float>(MAP_OFFSET_X + col * TILE_SIZE + TILE_SIZE / 2),
            static_cast<float>(MAP_OFFSET_Y + row * TILE_SIZE + TILE_SIZE / 2)};
}

bool screenToGrid(Vector2 screenPos, int& outRow, int& outCol) {
    int col = static_cast<int>((screenPos.x - MAP_OFFSET_X) / TILE_SIZE);
    int row = static_cast<int>((screenPos.y - MAP_OFFSET_Y) / TILE_SIZE);
    if (col < 0 || col >= MAP_COLS || row < 0 || row >= MAP_ROWS) return false;
    outRow = row;
    outCol = col;
    return true;
}

void drawMap(const Block& block) {
    const auto& grid = block.getGrid();
    for (int row = 0; row < static_cast<int>(grid.size()); ++row) {
        for (int col = 0; col < static_cast<int>(grid[row].size()); ++col) {
            float x = static_cast<float>(MAP_OFFSET_X + col * TILE_SIZE);
            float y = static_cast<float>(MAP_OFFSET_Y + row * TILE_SIZE);
            Color c = tileColor(grid[row][col].type);
            DrawRectangle(static_cast<int>(x), static_cast<int>(y),
                          TILE_SIZE, TILE_SIZE, c);
            DrawRectangleLines(static_cast<int>(x), static_cast<int>(y),
                               TILE_SIZE, TILE_SIZE, Color{40, 40, 40, 100});

            if (grid[row][col].hasTower) {
                DrawRectangle(static_cast<int>(x + 4), static_cast<int>(y + 4),
                              TILE_SIZE - 8, TILE_SIZE - 8,
                              Color{255, 255, 255, 40});
            }

            const char* label = nullptr;
            Color lc = WHITE;
            if (grid[row][col].type == TileType::Spawn) { label = "S"; lc = WHITE; }
            else if (grid[row][col].type == TileType::Base) { label = "B"; lc = WHITE; }
            if (label) {
                int tw = measureTextF(label, 20);
                drawTextF(label, static_cast<int>(x + TILE_SIZE / 2 - tw / 2),
                          static_cast<int>(y + TILE_SIZE / 2 - 10), 20, lc);
            }
        }
    }
}

void drawTower(const DefenseTower& tower, bool showRange, bool selected) {
    Vector2 center = {
        MAP_OFFSET_X + tower.getPosition().x,
        MAP_OFFSET_Y + tower.getPosition().y
    };

    if (showRange) {
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y),
                        tower.getRange(), Color{255, 255, 255, 80});
    }

    Color base = towerColor(tower.getName());
    DrawCircle(static_cast<int>(center.x), static_cast<int>(center.y),
               18, base);
    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y),
                    18, selected ? WHITE : Color{0, 0, 0, 180});
    DrawCircle(static_cast<int>(center.x), static_cast<int>(center.y),
               8, Color{255, 255, 255, 150});

    if (selected) {
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y),
                        22, YELLOW);
    }

    int fontSize = 10;
    const char* name = tower.getName().c_str();
    int tw = measureTextF(name, fontSize);
    drawTextF(name, static_cast<int>(center.x - tw / 2),
              static_cast<int>(center.y + 22), fontSize, WHITE);
}

void drawTowers(const std::vector<std::unique_ptr<DefenseTower>>& towers,
                int selectedIndex) {
    for (size_t i = 0; i < towers.size(); ++i) {
        drawTower(*towers[i], (static_cast<int>(i) == selectedIndex),
                  static_cast<int>(i) == selectedIndex);
    }
}

void drawEnemy(const Enemy& enemy) {
    if (enemy.getState() == EnemyState::DEAD) return;

    Rect box = enemy.getBoundingBox();
    Vector2 center = {
        MAP_OFFSET_X + box.x + box.width / 2.0f,
        MAP_OFFSET_Y + box.y + box.height / 2.0f
    };

    Color c = enemyColor(enemy.getName());
    float radius = 12.0f;

    if (enemy.getName().find("期中") != std::string::npos) {
        radius = 18.0f;
    }

    DrawCircle(static_cast<int>(center.x), static_cast<int>(center.y),
               radius, c);
    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y),
                    radius, Color{0, 0, 0, 180});

    float hpPct = static_cast<float>(enemy.getHp()) / enemy.getMaxHp();
    float barW = radius * 2.5f;
    float barH = 4.0f;
    float barX = center.x - barW / 2.0f;
    float barY = center.y - radius - 8.0f;

    DrawRectangle(static_cast<int>(barX), static_cast<int>(barY),
                  static_cast<int>(barW), static_cast<int>(barH),
                  Color{60, 60, 60, 200});
    DrawRectangle(static_cast<int>(barX), static_cast<int>(barY),
                  static_cast<int>(barW * hpPct), static_cast<int>(barH),
                  (hpPct > 0.5f) ? GREEN : (hpPct > 0.25f ? YELLOW : RED));

    if (enemy.getEffectiveMoveSpeed() < 1.0f) {
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y),
                        static_cast<int>(radius + 3), BLUE);
    }
}

void drawEnemies(const std::vector<Enemy*>& enemies) {
    for (const auto* e : enemies) {
        if (e) drawEnemy(*e);
    }
}

void drawTowerGuide(TowerKind selectedTower, int x, int y, int width,
                    int selectedTowerIndex) {
    const char* title = "塔说明";
    const char* name = "Coffee";
    std::vector<std::string> lines;

    switch (selectedTower) {
    case TowerKind::Coffee:
        name = "Coffee";
        lines = {
            "小范围高爆发单体塔。",
            "会锁定范围内最近的怪兽，",
            "适合放在路径拐角补刀。"
        };
        break;
    case TowerKind::AI:
        name = "AI";
        lines = {
            "范围内全体扫射。",
            "升级后伤害、射程和攻速提升。",
            "放下后点击该塔，按 U 升级。"
        };
        break;
    case TowerKind::Library:
        name = "Library";
        lines = {
            "不直接造成伤害。",
            "会让范围内怪兽减速，",
            "适合配合高伤害塔使用。"
        };
        break;
    case TowerKind::Class:
        name = "Class";
        lines = {
            "单次攻击很重。",
            "出手间隔较长，",
            "适合守住关键路口。"
        };
        break;
    case TowerKind::Bilibili:
        name = "Bilibili";
        lines = {
            "沿一个方向直线攻击。",
            "放置前按 R 切换朝向。",
            "放下后点击该塔，按 R 付费转向。"
        };
        break;
    }

    DrawRectangle(x, y, width, 150, Color{34, 34, 50, 220});
    DrawRectangleLines(x, y, width, 150, Color{80, 80, 115, 220});

    Color tc = towerColor(name);
    DrawCircle(x + 16, y + 20, 7, tc);
    drawTextF(title, x + 32, y + 10, 16, WHITE);
    drawTextF(name, x + width - measureTextF(name, 16) - 14, y + 10, 16, tc);

    int lineY = y + 42;
    for (const std::string& line : lines) {
        drawTextF(line.c_str(), x + 14, lineY, 13, LIGHTGRAY);
        lineY += 20;
    }

    if (selectedTowerIndex < 0) {
        drawTextF("点击高台格子放置当前塔。", x + 14, y + 124, 12, GRAY);
    } else {
        drawTextF("已选中地图上的塔。", x + 14, y + 124, 12, GRAY);
    }
}

void drawExerciseGuide(int x, int y, int width) {
    DrawRectangle(x, y, width, 150, Color{34, 42, 50, 220});
    DrawRectangleLines(x, y, width, 150, Color{80, 120, 95, 220});

    DrawCircle(x + 16, y + 20, 7, GREEN);
    drawTextF("运动模式", x + 32, y + 10, 16, WHITE);
    drawTextF("Exercise", x + width - measureTextF("Exercise", 16) - 14,
              y + 10, 16, GREEN);

    std::vector<std::string> lines = {
        "开启后会慢慢恢复身体健康。",
        "代价是防御塔火力节奏下降，",
        "适合身体指标接近阈值时救急。"
    };

    int lineY = y + 42;
    for (const std::string& line : lines) {
        drawTextF(line.c_str(), x + 14, lineY, 13, LIGHTGRAY);
        lineY += 20;
    }

    drawTextF("再次点击 Exercise 可关闭。", x + 14, y + 124, 12, GRAY);
}

void drawUI(const GameSnapshot& snap, int gold, TowerKind selectedTower,
            bool exerciseMode, int selectedTowerIndex, bool showExerciseGuide,
            float timeScale) {
    int x = UI_PANEL_X;
    int w = UI_PANEL_WIDTH;

    DrawRectangle(x, 0, w, SCREEN_HEIGHT, Color{30, 30, 40, 240});
    DrawRectangleLines(x, 0, w, SCREEN_HEIGHT, Color{80, 80, 100, 255});

    int y = 20;
    const int lx = x + 15;

    drawTextF(GameEngine::phaseName(snap.phase), lx, y, 22, phaseColor(snap.phase));
    y += 30;

    drawTextF(("Gold: " + std::to_string(gold)).c_str(), lx, y, 20, GOLD);
    y += 10;
    drawTextF(("Wave: " + std::to_string(snap.waveIndex + 1) + "/3").c_str(),
              lx, y, 16, LIGHTGRAY);
    y += 10;
    drawTextF(("Level: " + std::to_string(snap.levelIndex) + "/4").c_str(),
              lx, y, 16, LIGHTGRAY);
    y += 20;

    DrawLine(x + 10, y, x + w - 10, y, Color{80, 80, 100, 200});
    y += 12;

    const char* statNames[] = {"Academic", "Physical", "Mental", "Connection"};
    int stats[] = {snap.currentAcademic, snap.currentPhysical,
                   snap.currentMental, snap.currentConnection};
    int thresholds[] = {snap.thresholdAcademic, snap.thresholdPhysical,
                        snap.thresholdMental, snap.thresholdConnection};

    drawTextF("Player Stats", lx, y, 16, LIGHTGRAY);
    y += 22;

    for (int i = 0; i < 4; ++i) {
        Color ic = indicatorColor(i);
        drawTextF(statNames[i], lx, y, 14, ic);
        drawTextF(TextFormat("%d", stats[i]), lx + 100, y, 14, ic);

        int barX = lx + 130;
        int barW = 160;
        DrawRectangle(barX, y + 4, barW, 10, Color{50, 50, 50, 200});
        float pct = stats[i] / 100.0f;
        DrawRectangle(barX, y + 4, static_cast<int>(barW * pct), 10, ic);
        float tpct = thresholds[i] / 100.0f;
        int tx = barX + static_cast<int>(barW * tpct);
        DrawRectangle(tx - 1, y + 2, 3, 14, WHITE);

        y += 20;
    }

    y += 8;
    DrawLine(x + 10, y, x + w - 10, y, Color{80, 80, 100, 200});
    y += 12;

    bool canBuild = (snap.phase == GamePhase::Build ||
                     snap.phase == GamePhase::WaveCleared);

    if (canBuild) {
        drawTextF("Towers (click to select)", lx, y, 16, LIGHTGRAY);
        y += 24;

        TowerKind kinds[] = {TowerKind::Coffee, TowerKind::AI, TowerKind::Library,
                            TowerKind::Class, TowerKind::Bilibili};
        for (int i = 0; i < 5; ++i) {
            const TowerSpec spec = GameEngine::towerSpec(kinds[i]);
            Rectangle btn = {static_cast<float>(lx), static_cast<float>(y),
                             static_cast<float>(w - 30), 26.0f};
            bool hover = CheckCollisionPointRec(GetMousePosition(), btn);
            bool sel = (selectedTower == kinds[i]);

            Color bg = sel ? Color{60, 60, 100, 255} : (hover ? Color{50, 50, 70, 255} : Color{40, 40, 55, 255});
            DrawRectangleRec(btn, bg);
            DrawRectangleLinesEx(btn, 1, sel ? WHITE : Color{80, 80, 100, 200});

            Color tc = towerColor(spec.name);
            DrawCircle(lx + 14, y + 13, 6, tc);
            std::string towerLabel = std::string(spec.name) + "  " + std::to_string(spec.cost) + "g";
            drawTextF(towerLabel.c_str(), lx + 26, y + 4, 16, (gold >= spec.cost) ? tc : GRAY);

            y += 30;
        }

        y += 8;
        DrawLine(x + 10, y, x + w - 10, y, Color{80, 80, 100, 200});
        y += 12;
    }

    if (canBuild) {
        Rectangle exBtn = {static_cast<float>(lx), static_cast<float>(y),
                           static_cast<float>(w - 30), 30.0f};
        bool exHover = CheckCollisionPointRec(GetMousePosition(), exBtn);
        DrawRectangleRec(exBtn, exHover ? Color{60, 60, 80, 255} : Color{45, 45, 60, 255});
        drawTextF(exerciseMode ? "Exercise: ON" : "Exercise: OFF",
                  lx + 10, y + 6, 16, exerciseMode ? GREEN : GRAY);
        y += 38;
    }

    if (canBuild) {
        Rectangle swBtn = {static_cast<float>(lx), static_cast<float>(y),
                           static_cast<float>(w - 30), 36.0f};
        bool swHover = CheckCollisionPointRec(GetMousePosition(), swBtn);
        DrawRectangleRec(swBtn, swHover ? Color{40, 120, 40, 255} : Color{30, 90, 30, 255});
        drawTextF("Start Wave", lx + 40, y + 8, 20, WHITE);
        y += 44;
    }

    y += 10;
    DrawLine(x + 10, y, x + w - 10, y, Color{80, 80, 100, 200});
    y += 12;

    if (canBuild) {
        if (showExerciseGuide) {
            drawExerciseGuide(lx, y, w - 30);
        } else {
            drawTowerGuide(selectedTower, lx, y, w - 30, selectedTowerIndex);
        }
        y += 162;
    }

    if (exerciseMode) {
        drawTextF("Exercise: +1 Physical/3s", lx, y, 12, GREEN);
        y += 16;
        drawTextF("Tower DPS at 65%", lx, y, 12, Color{255, 200, 100, 255});
    }

    // 时间缩放显示
    if (timeScale != 1.0f) {
        y += 20;
        char speedBuf[64];
        snprintf(speedBuf, sizeof(speedBuf), "Speed: %.1fx", timeScale);
        drawTextF(speedBuf, lx, y, 14, Color{255, 200, 50, 255});
    }
}

void drawMainMenu() {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{20, 20, 35, 255});

    const char* title = "GPA Defender";
    int tw = measureTextF(title, 48);
    drawTextF(title, SCREEN_WIDTH / 2 - tw / 2, 200, 48, WHITE);

    const char* subtitle = "Tower Defense for Academic Survival";
    tw = measureTextF(subtitle, 20);
    drawTextF(subtitle, SCREEN_WIDTH / 2 - tw / 2, 270, 20, LIGHTGRAY);

    const char* prompt = "Press ENTER to Start";
    tw = measureTextF(prompt, 22);

    float alpha = 0.5f + 0.5f * sinf(static_cast<float>(GetTime()) * 3.0f);
    drawTextF(prompt, SCREEN_WIDTH / 2 - tw / 2, 450, 22,
              Color{255, 255, 255, static_cast<unsigned char>(alpha * 255)});

    const char* info = "Defend your GPA! Place towers, survive waves,";
    tw = measureTextF(info, 14);
    drawTextF(info, SCREEN_WIDTH / 2 - tw / 2, 550, 14, GRAY);

    info = "and keep all four indicators above threshold.";
    tw = measureTextF(info, 14);
    drawTextF(info, SCREEN_WIDTH / 2 - tw / 2, 570, 14, GRAY);
}

void drawLevelSelect(int unlockedLevel, int hoveredLevel) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{20, 20, 35, 255});

    const char* title = "选择关卡";
    int tw = measureTextF(title, 42);
    drawTextF(title, SCREEN_WIDTH / 2 - tw / 2, 72, 42, WHITE);

    const char* subtitle = "完成关卡后会解锁下一关";
    tw = measureTextF(subtitle, 18);
    drawTextF(subtitle, SCREEN_WIDTH / 2 - tw / 2, 128, 18, LIGHTGRAY);

    const int cardW = 190;
    const int cardH = 170;
    const int gap = 35;
    const int totalW = cardW * 4 + gap * 3;
    const int startX = SCREEN_WIDTH / 2 - totalW / 2;
    const int cardY = 230;
    const char* levelNames[] = {"大一", "大二", "大三", "大四"};

    for (int i = 0; i < 4; ++i) {
        int level = i + 1;
        bool unlocked = level <= unlockedLevel;
        bool hovered = hoveredLevel == level && unlocked;

        float scale = hovered ? 1.08f : 1.0f;
        int w = static_cast<int>(cardW * scale);
        int h = static_cast<int>(cardH * scale);
        int x = startX + i * (cardW + gap) - (w - cardW) / 2;
        int y = cardY - (h - cardH) / 2;

        Color bg = unlocked ? Color{36, 36, 58, 255} : Color{26, 26, 38, 255};
        Color border = hovered ? Color{255, 220, 100, 255}
                               : (unlocked ? Color{90, 90, 140, 255} : Color{70, 70, 85, 255});
        DrawRectangle(x, y, w, h, bg);
        DrawRectangleLines(x, y, w, h, border);

        char levelBuf[32];
        snprintf(levelBuf, sizeof(levelBuf), "LEVEL %d", level);
        tw = measureTextF(levelBuf, 16);
        drawTextF(levelBuf, x + w / 2 - tw / 2, y + 30, 16,
                  unlocked ? Color{180, 190, 230, 255} : Color{95, 95, 110, 255});

        tw = measureTextF(levelNames[i], 34);
        drawTextF(levelNames[i], x + w / 2 - tw / 2, y + 66, 34,
                  unlocked ? WHITE : Color{110, 110, 125, 255});

        const char* status = unlocked ? "已解锁" : "LOCKED";
        tw = measureTextF(status, unlocked ? 17 : 20);
        drawTextF(status, x + w / 2 - tw / 2, y + 122, unlocked ? 17 : 20,
                  unlocked ? Color{90, 220, 130, 255} : Color{210, 85, 85, 255});
    }

    Rectangle retryRect{
        SCREEN_WIDTH / 2.0f - 140.0f,
        660.0f,
        280.0f,
        48.0f
    };
    bool retryHovered = CheckCollisionPointRec(GetMousePosition(), retryRect);
    Color retryBg = retryHovered ? Color{60, 60, 92, 255} : Color{38, 38, 58, 255};
    Color retryBorder = retryHovered ? Color{255, 220, 100, 255} : Color{90, 90, 130, 255};
    DrawRectangleRec(retryRect, retryBg);
    DrawRectangleLinesEx(retryRect, 1.0f, retryBorder);

    const char* retry = "Retry ASTI test";
    tw = measureTextF(retry, 20);
    drawTextF(retry,
              static_cast<int>(retryRect.x + retryRect.width / 2 - tw / 2),
              static_cast<int>(retryRect.y + 13),
              20,
              retryHovered ? WHITE : LIGHTGRAY);
}

void drawGameOver(int selection) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                  Color{0, 0, 0, 180});

    const char* msg = "GAME OVER";
    int tw = measureTextF(msg, 52);
    drawTextF(msg, SCREEN_WIDTH / 2 - tw / 2, 260, 52, Color{220, 60, 60, 255});

    const char* hint = "Your Score has fallen below threshold...";
    tw = measureTextF(hint, 18);
    drawTextF(hint, SCREEN_WIDTH / 2 - tw / 2, 340, 18, LIGHTGRAY);

    const char* retry = "Retry This Level";
    const char* menu = "Return to Main Menu";

    Color retryColor = (selection == 0) ? Color{255, 220, 100, 255} : GRAY;
    Color menuColor  = (selection == 1) ? Color{255, 220, 100, 255} : GRAY;

    tw = measureTextF(retry, 20);
    drawTextF(retry, SCREEN_WIDTH / 2 - tw / 2, 400, 20, retryColor);

    tw = measureTextF(menu, 20);
    drawTextF(menu, SCREEN_WIDTH / 2 - tw / 2, 440, 20, menuColor);

    hint = "Use UP/DOWN to select, ENTER to confirm";
    tw = measureTextF(hint, 14);
    drawTextF(hint, SCREEN_WIDTH / 2 - tw / 2, 500, 14, DARKGRAY);
}

void drawVictory(int selection, bool hasNextLevel) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                  Color{0, 0, 0, 180});

    const char* msg = "VICTORY!";
    int tw = measureTextF(msg, 52);
    drawTextF(msg, SCREEN_WIDTH / 2 - tw / 2, 260, 52, Color{80, 220, 80, 255});

    const char* hint = "You have successfully defended your GPA!";
    tw = measureTextF(hint, 18);
    drawTextF(hint, SCREEN_WIDTH / 2 - tw / 2, 340, 18, LIGHTGRAY);

    const char* nextLevel = "Continue to Next Level";
    const char* menu = "Return to Main Menu";

    if (hasNextLevel) {
        Color nextColor = (selection == 0) ? Color{255, 220, 100, 255} : GRAY;
        Color menuColor = (selection == 1) ? Color{255, 220, 100, 255} : GRAY;

        tw = measureTextF(nextLevel, 20);
        drawTextF(nextLevel, SCREEN_WIDTH / 2 - tw / 2, 400, 20, nextColor);

        tw = measureTextF(menu, 20);
        drawTextF(menu, SCREEN_WIDTH / 2 - tw / 2, 440, 20, menuColor);
    } else {
        Color menuColor = (selection == 0) ? Color{255, 220, 100, 255} : GRAY;
        tw = measureTextF(menu, 20);
        drawTextF(menu, SCREEN_WIDTH / 2 - tw / 2, 400, 20, menuColor);
    }

    hint = "Use UP/DOWN to select, ENTER to confirm";
    tw = measureTextF(hint, 14);
    drawTextF(hint, SCREEN_WIDTH / 2 - tw / 2, 500, 14, DARKGRAY);
}

void drawQuestionnaire(const Questionnaire& q, int current,
                       const std::vector<int>& answers) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{20, 20, 35, 255});

    const auto& questions = q.getQuestions();
    if (current < 0 || current >= static_cast<int>(questions.size())) return;

    const auto& question = questions[current];

    const char* title = "ASTI Personality Quiz";
    int tw = measureTextF(title, 32);
    drawTextF(title, SCREEN_WIDTH / 2 - tw / 2, 40, 32, WHITE);

    std::string progress = "Question " + std::to_string(current + 1) + " / " +
                           std::to_string(static_cast<int>(questions.size()));
    tw = measureTextF(progress.c_str(), 16);
    drawTextF(progress.c_str(), SCREEN_WIDTH / 2 - tw / 2, 80, 16, GRAY);

    DrawRectangle(60, 125, SCREEN_WIDTH - 120, 86, Color{30, 30, 50, 255});
    DrawRectangleLines(60, 125, SCREEN_WIDTH - 120, 86, Color{80, 80, 120, 255});
    drawTextF(question.prompt.c_str(), 80, 149, 22, WHITE);

    for (size_t i = 0; i < question.options.size(); ++i) {
        const int optionHeight = 74;
        const int optionStep = 88;
        int oy = 235 + static_cast<int>(i) * optionStep;
        bool selected = (current < static_cast<int>(answers.size()) &&
                         answers[current] == static_cast<int>(i));
        Color bg = selected ? Color{50, 50, 100, 255} : Color{35, 35, 55, 255};
        DrawRectangle(100, oy, SCREEN_WIDTH - 200, optionHeight, bg);
        DrawRectangleLines(100, oy, SCREEN_WIDTH - 200, optionHeight,
                           selected ? WHITE : Color{80, 80, 120, 255});

        std::string label = std::to_string(i + 1) + ".";
        drawTextF(label.c_str(), 120, oy + 11, 18, LIGHTGRAY);

        const std::vector<std::string> optionLines =
            splitTextLines(question.options[i].text);
        drawTextLinesF(optionLines, 150, oy + 9, 18, 22,
                       selected ? WHITE : LIGHTGRAY);

        std::string effStr;
        for (const auto& eff : question.options[i].effects) {
            const char* indName[] = {"Academic", "Physical", "Mental", "Connection"};
            if (!effStr.empty()) effStr += ", ";
            char sign = eff.delta >= 0 ? '+' : '-';
            effStr += std::string(indName[static_cast<int>(eff.indicator)]) +
                      " " + sign + std::to_string(abs(eff.delta));
        }
        drawTextF(effStr.c_str(), 150, oy + optionHeight - 21, 12, GRAY);
    }

    const char* hint = "Press 1-4 to select an answer";
    tw = measureTextF(hint, 16);
    drawTextF(hint, SCREEN_WIDTH / 2 - tw / 2, SCREEN_HEIGHT - 60, 16, GRAY);
}

void drawAstiSummary(const AstiResult& result) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{20, 20, 35, 255});

    const char* title = "你的 ASTI 类型";
    int tw = measureTextF(title, 38);
    drawTextF(title, SCREEN_WIDTH / 2 - tw / 2, 72, 38, WHITE);

    const char* subtitle = "Attend School Type Indicator 上学人格。";
    tw = measureTextF(subtitle, 22);
    drawTextF(subtitle, SCREEN_WIDTH / 2 - tw / 2, 124, 22, LIGHTGRAY);

    DrawRectangle(120, 190, SCREEN_WIDTH - 240, 130, Color{35, 35, 58, 255});
    DrawRectangleLines(120, 190, SCREEN_WIDTH - 240, 130, Color{90, 90, 135, 255});

    drawTextF("结果类型", 155, 214, 24, WHITE);

    std::string tagText;
    for (size_t i = 0; i < result.tags.size(); ++i) {
        if (!tagText.empty()) tagText += " / ";
        tagText += result.tags[i];
    }
    if (tagText.empty()) tagText = "未识别";

    tw = measureTextF(tagText.c_str(), 34);
    drawTextF(tagText.c_str(), SCREEN_WIDTH / 2 - tw / 2, 254, 34, GOLD);

    const int cardX = 150;
    const int cardY = 380;
    const int cardW = 900;
    const int rowH = 54;

    drawTextF("最终阈值分数", cardX, 346, 24, WHITE);
    drawTextF("被怪兽攻击至该分数以下则失败", cardX + 160, 350, 19, LIGHTGRAY);

    const char* names[] = {"学业成绩", "身体健康", "心理健康", "联结感"};
    int values[] = {
        result.thresholdAcademic,
        result.thresholdPhysical,
        result.thresholdMental,
        result.thresholdConnection
    };

    for (int i = 0; i < 4; ++i) {
        int y = cardY + i * rowH;
        Color c = indicatorColor(i);
        DrawRectangle(cardX, y, cardW, rowH - 8,
                      (i % 2 == 0) ? Color{32, 32, 50, 255} : Color{38, 38, 58, 255});
        DrawRectangleLines(cardX, y, cardW, rowH - 8, Color{75, 75, 105, 210});
        DrawCircle(cardX + 28, y + 23, 8, c);
        drawTextF(names[i], cardX + 52, y + 12, 20, LIGHTGRAY);

        std::string valueStr = std::to_string(values[i]);
        int valueW = measureTextF(valueStr.c_str(), 24);
        drawTextF(valueStr.c_str(), cardX + cardW - valueW - 34, y + 10, 24, c);
    }

    const char* hint = "按 Enter 继续进入游戏";
    tw = measureTextF(hint, 20);
    float alpha = 0.5f + 0.5f * sinf(static_cast<float>(GetTime()) * 3.0f);
    drawTextF(hint, SCREEN_WIDTH / 2 - tw / 2, SCREEN_HEIGHT - 82, 20,
              Color{255, 255, 255, static_cast<unsigned char>(alpha * 255)});
}

void drawChests(const std::vector<Chest>& chests) {
    constexpr float kChestLifetime = 15.0f;
    constexpr int kChestWidth = 32;
    constexpr int kChestHeight = 24;
    constexpr int kChestLabelSize = 16;
    constexpr int kTimerBarHeight = 4;

    for (const Chest& chest : chests) {
        if (chest.state != ChestState::Active) continue;

        int x = static_cast<int>(chest.position.x + MAP_OFFSET_X);
        int y = static_cast<int>(chest.position.y + MAP_OFFSET_Y + chest.bounceOffset);

        Color color = chestColor(chest.type);
        const char* label = chestLabel(chest.type);

        // 绘制宝箱底座
        DrawRectangle(x - kChestWidth / 2, y - kChestHeight / 2,
                      kChestWidth, kChestHeight, color);
        DrawRectangleLines(x - kChestWidth / 2, y - kChestHeight / 2,
                           kChestWidth, kChestHeight, WHITE);

        // 绘制标签
        int tw = measureTextF(label, kChestLabelSize);
        drawTextF(label, x - tw / 2, y - kChestLabelSize / 2,
                  kChestLabelSize, WHITE);

        // 绘制倒计时条
        float ratio = chest.timer / kChestLifetime;
        DrawRectangle(x - kChestWidth / 2, y + kChestHeight / 2 + 2,
                      kChestWidth, kTimerBarHeight, Color{50, 50, 50, 200});
        DrawRectangle(x - kChestWidth / 2, y + kChestHeight / 2 + 2,
                      static_cast<int>(kChestWidth * ratio), kTimerBarHeight,
                      ratio > 0.3f ? GREEN : RED);
    }
}

}  // namespace frontend
