#include "frontend/Renderer.h"
#include "frontend/TextureManager.h"

#include "gpa_defender/PlayerStats.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace frontend {

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

// ---- Sprite helpers ----

Rectangle mapTileSrc(int col, int row) {
    return {static_cast<float>(col * 64), static_cast<float>(row * 64), 64.0f, 64.0f};
}

const char* towerSpriteName(const std::string& towerName) {
    if (towerName.find("Coffee") != std::string::npos)   return "ai_coffee";
    if (towerName.find("AI") != std::string::npos)        return "ai_ai";
    if (towerName.find("Library") != std::string::npos)   return "ai_library";
    if (towerName.find("Class") != std::string::npos)     return "ai_class";
    if (towerName.find("Bilibili") != std::string::npos)  return "ai_bilibili";
    return "ai_coffee";
}

const char* enemySpriteName(const std::string& enemyName) {
    if (enemyName.find("期中") != std::string::npos || enemyName.find("Midterm") != std::string::npos)
        return "ai_boss";
    if (enemyName.find("微积分") != std::string::npos || enemyName.find("线代") != std::string::npos ||
        enemyName.find("Calculus") != std::string::npos)
        return "ai_calculus";
    if (enemyName.find("科研") != std::string::npos || enemyName.find("Research") != std::string::npos)
        return "ai_research";
    if (enemyName.find("社交") != std::string::npos || enemyName.find("Friends") != std::string::npos)
        return "ai_social";
    if (enemyName.find("早八") != std::string::npos || enemyName.find("Morning") != std::string::npos)
        return "ai_morning";
    if (enemyName.find("小组") != std::string::npos || enemyName.find("Group") != std::string::npos)
        return "ai_group";
    if (enemyName.find("短视频") != std::string::npos || enemyName.find("Video") != std::string::npos)
        return "ai_video";
    if (enemyName.find("考纲") != std::string::npos || enemyName.find("Syllabus") != std::string::npos)
        return "ai_exam_outline";
    if (enemyName.find("同辈") != std::string::npos || enemyName.find("压力") != std::string::npos ||
        enemyName.find("Pressure") != std::string::npos)
        return "ai_peer_pressure";
    return "ai_calculus";
}

void drawSprite(const TextureManager& tm, const char* name,
                float cx, float cy, float scale, Color tint) {
    const SpriteDef* def = tm.getSprite(name);
    if (!def || !def->texture) return;
    float w = def->src.width * scale;
    float h = def->src.height * scale;
    Rectangle dest = {cx - w / 2.0f, cy - h / 2.0f, w, h};
    DrawTexturePro(*def->texture, def->src, dest, {0, 0}, 0.0f, tint);
}

// ---- Map ----

void drawMap(const Block& block, const TextureManager* tm) {
    const auto& grid = block.getGrid();
    for (int row = 0; row < static_cast<int>(grid.size()); ++row) {
        for (int col = 0; col < static_cast<int>(grid[row].size()); ++col) {
            float x = static_cast<float>(MAP_OFFSET_X + col * TILE_SIZE);
            float y = static_cast<float>(MAP_OFFSET_Y + row * TILE_SIZE);
            TileType type = grid[row][col].type;

            if (tm && tm->mapTilesheet.id != 0) {
                Rectangle src;
                switch (type) {
                case TileType::Highland: src = mapTileSrc(0, 0); break;
                case TileType::Path:     src = mapTileSrc(9, 2); break;
                case TileType::Spawn:    src = mapTileSrc(8, 9); break;
                case TileType::Base:     src = mapTileSrc(12, 3); break;
                case TileType::Wall:
                default:                 src = mapTileSrc(0, 0); break;
                }
                DrawTexturePro(tm->mapTilesheet, src,
                               {x, y, static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)},
                               {0, 0}, 0.0f, WHITE);
            } else {
                Color c = tileColor(type);
                DrawRectangle(static_cast<int>(x), static_cast<int>(y),
                              TILE_SIZE, TILE_SIZE, c);
            }

            DrawRectangleLines(static_cast<int>(x), static_cast<int>(y),
                               TILE_SIZE, TILE_SIZE, Color{40, 40, 40, 100});

            if (grid[row][col].hasTower) {
                DrawRectangle(static_cast<int>(x + 4), static_cast<int>(y + 4),
                              TILE_SIZE - 8, TILE_SIZE - 8,
                              Color{255, 255, 255, 40});
            }

            const char* label = nullptr;
            Color lc = WHITE;
            if (type == TileType::Spawn) { label = "S"; lc = WHITE; }
            else if (type == TileType::Base) { label = "B"; lc = WHITE; }
            if (label) {
                int tw = measureTextF(label, 26);
                drawTextF(label, static_cast<int>(x + TILE_SIZE / 2 - tw / 2),
                          static_cast<int>(y + TILE_SIZE / 2 - 13), 26, lc);
            }
        }
    }
}

// ---- Towers ----

void drawTower(const DefenseTower& tower, bool showRange, bool selected,
               const TextureManager* tm) {
    Vector2 center = {
        MAP_OFFSET_X + tower.getPosition().x,
        MAP_OFFSET_Y + tower.getPosition().y
    };

    if (showRange) {
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y),
                        tower.getRange(), Color{255, 255, 255, 80});
    }

    if (tm) {
        const char* sprite = towerSpriteName(tower.getName());
        drawSprite(*tm, "shadow", center.x, center.y + 10, 0.715f, Color{0, 0, 0, 120});
        drawSprite(*tm, sprite, center.x, center.y - 26, 0.08385f, WHITE);
    } else {
        Color base = towerColor(tower.getName());
        DrawCircle(static_cast<int>(center.x), static_cast<int>(center.y), 23, base);
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y),
                        23, selected ? WHITE : Color{0, 0, 0, 180});
        DrawCircle(static_cast<int>(center.x), static_cast<int>(center.y),
                   10, Color{255, 255, 255, 150});
    }

    if (selected) {
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y),
                        29, YELLOW);
    }

    int fontSize = 13;
    const char* name = tower.getName().c_str();
    int tw = measureTextF(name, fontSize);
    drawTextF(name, static_cast<int>(center.x - tw / 2),
              static_cast<int>(center.y + 29), fontSize, WHITE);
}

void drawTowers(const std::vector<std::unique_ptr<DefenseTower>>& towers,
                int selectedIndex, const TextureManager* tm) {
    for (size_t i = 0; i < towers.size(); ++i) {
        drawTower(*towers[i], (static_cast<int>(i) == selectedIndex),
                  static_cast<int>(i) == selectedIndex, tm);
    }
}

// ---- Enemies ----

void drawEnemy(const Enemy& enemy, const TextureManager* tm) {
    if (enemy.getState() == EnemyState::DEAD) return;

    Rect box = enemy.getBoundingBox();
    Vector2 center = {
        MAP_OFFSET_X + box.x + box.width / 2.0f,
        MAP_OFFSET_Y + box.y + box.height / 2.0f
    };

    float scale = 0.08385f;

    if (tm) {
        const char* sprite = enemySpriteName(enemy.getName());
        drawSprite(*tm, "shadow", center.x, center.y + 8, scale * 0.9f, Color{0, 0, 0, 120});
        drawSprite(*tm, sprite, center.x, center.y - 26, scale, WHITE);
    } else {
        Color c = enemyColor(enemy.getName());
        float radius = 28.6f;
        DrawCircle(static_cast<int>(center.x), static_cast<int>(center.y), radius, c);
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y),
                        radius, Color{0, 0, 0, 180});
    }

    // HP bar
    float hpPct = static_cast<float>(enemy.getHp()) / enemy.getMaxHp();
    float barW = 65.0f;
    float barH = 5.0f;
    float barX = center.x - barW / 2.0f;
    float barY = center.y - 96.0f;

    DrawRectangle(static_cast<int>(barX), static_cast<int>(barY),
                  static_cast<int>(barW), static_cast<int>(barH),
                  Color{60, 60, 60, 200});
    DrawRectangle(static_cast<int>(barX), static_cast<int>(barY),
                  static_cast<int>(barW * hpPct), static_cast<int>(barH),
                  (hpPct > 0.5f) ? GREEN : (hpPct > 0.25f ? YELLOW : RED));

    if (enemy.getEffectiveMoveSpeed() < 1.0f) {
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y),
                        90, BLUE);
    }
}

void drawEnemies(const std::vector<Enemy*>& enemies, const TextureManager* tm) {
    for (const auto* e : enemies) {
        if (e) drawEnemy(*e, tm);
    }
}

// ---- Chests ----

void drawChests(const std::vector<Chest>& chests, const TextureManager* tm) {
    constexpr float kChestLifetime = 15.0f;
    constexpr int kChestWidth = 42;
    constexpr int kChestHeight = 31;
    constexpr int kChestLabelSize = 21;
    constexpr int kTimerBarHeight = 5;

    for (const Chest& chest : chests) {
        if (chest.state != ChestState::Active) continue;

        int x = static_cast<int>(chest.position.x + MAP_OFFSET_X);
        int y = static_cast<int>(chest.position.y + MAP_OFFSET_Y + chest.bounceOffset);

        if (tm) {
            Color cc = chestColor(chest.type);
            drawSprite(*tm, "tile_coin", static_cast<float>(x), static_cast<float>(y),
                       0.585f, cc);
        } else {
            Color color = chestColor(chest.type);
            const char* label = chestLabel(chest.type);
            DrawRectangle(x - kChestWidth / 2, y - kChestHeight / 2,
                          kChestWidth, kChestHeight, color);
            DrawRectangleLines(x - kChestWidth / 2, y - kChestHeight / 2,
                               kChestWidth, kChestHeight, WHITE);
            int tw = measureTextF(label, kChestLabelSize);
            drawTextF(label, x - tw / 2, y - kChestLabelSize / 2,
                      kChestLabelSize, WHITE);
        }

        // Timer bar
        float ratio = chest.timer / kChestLifetime;
        DrawRectangle(x - kChestWidth / 2, y + kChestHeight / 2 + 2,
                      kChestWidth, kTimerBarHeight, Color{50, 50, 50, 200});
        DrawRectangle(x - kChestWidth / 2, y + kChestHeight / 2 + 2,
                      static_cast<int>(kChestWidth * ratio), kTimerBarHeight,
                      ratio > 0.3f ? GREEN : RED);
    }
}

// ---- UI helpers ----

void drawTowerGuide(TowerKind selectedTower, int x, int y, int width,
                    int selectedTowerIndex, const TextureManager* tm) {
    const char* title = "塔说明";
    const char* name = "Coffee";
    std::vector<std::string> lines;

    switch (selectedTower) {
    case TowerKind::Coffee:
        name = "Coffee";
        lines = {"小范围高爆发单体塔。", "会锁定范围内最近的怪兽，", "适合放在路径拐角补刀。"};
        break;
    case TowerKind::AI:
        name = "AI";
        lines = {"范围内全体扫射。", "升级后伤害、射程和攻速提升。", "放下后点击该塔，按 U 升级。"};
        break;
    case TowerKind::Library:
        name = "Library";
        lines = {"不直接造成伤害。", "会让范围内怪兽减速，", "适合配合高伤害塔使用。"};
        break;
    case TowerKind::Class:
        name = "Class";
        lines = {"单次攻击很重。", "出手间隔较长，", "适合守住关键路口。"};
        break;
    case TowerKind::Bilibili:
        name = "Bilibili";
        lines = {"沿一个方向直线攻击。", "放置前按 R 切换朝向。", "放下后点击该塔，按 R 付费转向。"};
        break;
    }

    DrawRectangleRounded(
        {static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), 255.0f},
        0.2f, 8, Color{34, 34, 50, 220});
    DrawRectangleRoundedLines(
        {static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), 255.0f},
        0.2f, 8, 1.0f, Color{80, 80, 115, 220});

    if (tm) {
        const char* sprite = towerSpriteName(name);
        drawSprite(*tm, sprite, static_cast<float>(x + 33), static_cast<float>(y + 39), 0.024f, WHITE);
    } else {
        Color tc = towerColor(name);
        DrawCircle(x + 24, y + 30, 12, tc);
    }
    drawTextF(title, x + 55, y + 18, 30, WHITE);
    drawTextF(name, x + width - measureTextF(name, 30) - 20, y + 18, 30, towerColor(name));

    int lineY = y + 72;
    for (const std::string& line : lines) {
        drawTextF(line.c_str(), x + 20, lineY, 24, LIGHTGRAY);
        lineY += 36;
    }

    if (selectedTowerIndex < 0) {
        drawTextF("点击高台格子放置当前塔。", x + 20, y + 210, 20, GRAY);
    } else {
        drawTextF("已选中地图上的塔。", x + 20, y + 210, 20, GRAY);
    }
}

void drawExerciseGuide(int x, int y, int width) {
    DrawRectangleRounded(
        {static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), 255.0f},
        0.2f, 8, Color{34, 42, 50, 220});
    DrawRectangleRoundedLines(
        {static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), 255.0f},
        0.2f, 8, 1.0f, Color{80, 120, 95, 220});

    DrawCircle(x + 24, y + 33, 12, GREEN);
    drawTextF("运动模式", x + 50, y + 18, 30, WHITE);
    drawTextF("Exercise", x + width - measureTextF("Exercise", 30) - 20,
              y + 18, 30, GREEN);

    std::vector<std::string> lines = {
        "开启后会慢慢恢复身体健康。",
        "代价是防御塔火力节奏下降，",
        "适合身体指标接近阈值时救急。"
    };

    int lineY = y + 72;
    for (const std::string& line : lines) {
        drawTextF(line.c_str(), x + 20, lineY, 24, LIGHTGRAY);
        lineY += 36;
    }

    drawTextF("再次点击 Exercise 可关闭。", x + 20, y + 210, 20, GRAY);
}

// ---- Hover preview ----

void drawHoverPreview(int hoveredRow, int hoveredCol,
                      TowerKind selectedTower, const Vector2& bilibiliDir,
                      const Block& block, const TextureManager* tm) {
    if (hoveredRow < 0 || hoveredCol < 0) return;
    if (!block.canPlaceTower(hoveredRow, hoveredCol)) return;

    Vector2 center = gridToScreen(hoveredRow, hoveredCol);
    const TowerSpec spec = GameEngine::towerSpec(selectedTower);
    Color prevColor = towerColor(spec.name);

    DrawRectangle(static_cast<int>(center.x - TILE_SIZE / 2 + 2),
                  static_cast<int>(center.y - TILE_SIZE / 2 + 2),
                  TILE_SIZE - 4, TILE_SIZE - 4,
                  Color{prevColor.r, prevColor.g, prevColor.b, 80});

    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y),
                    spec.range, Color{255, 255, 255, 60});

    if (tm) {
        const char* sprite = towerSpriteName(spec.name);
        drawSprite(*tm, sprite, center.x, center.y - 26, 0.06864f,
                   Color{255, 255, 255, 120});
    }

    if (selectedTower == TowerKind::Bilibili) {
        Vector2 end = {center.x + bilibiliDir.x * 52, center.y + bilibiliDir.y * 52};
        DrawLine(static_cast<int>(center.x), static_cast<int>(center.y),
                 static_cast<int>(end.x), static_cast<int>(end.y), PINK);
        DrawCircle(static_cast<int>(end.x), static_cast<int>(end.y), 7, PINK);
    }
}

// ---- Main drawUI ----

void drawUI(const GameSnapshot& snap, int gold, TowerKind selectedTower,
            bool exerciseMode, int selectedTowerIndex, bool showExerciseGuide,
            float timeScale, const TextureManager* tm) {
    int x = UI_PANEL_X;
    int w = UI_PANEL_WIDTH;

    DrawRectangleGradientV(x, 0, w, SCREEN_HEIGHT,
                           Color{30, 30, 45, 248}, Color{18, 18, 30, 248});
    DrawRectangleLines(x, 0, w, SCREEN_HEIGHT, Color{80, 80, 100, 255});

    int y = 26;
    const int lx = x + 20;

    drawTextF(GameEngine::phaseName(snap.phase), lx, y, 42, phaseColor(snap.phase));
    y += 54;

    // Gold with coin icon
    if (tm) {
        drawSprite(*tm, "tile_coin", static_cast<float>(lx + 15), static_cast<float>(y + 15),
                   0.60f, GOLD);
        drawTextF(("  " + std::to_string(gold)).c_str(), lx + 27, y, 40, GOLD);
    } else {
        drawTextF(("Gold: " + std::to_string(gold)).c_str(), lx, y, 40, GOLD);
    }
    y += 32;
    drawTextF(("Wave: " + std::to_string(snap.waveIndex + 1) + "/3").c_str(),
              lx, y, 30, LIGHTGRAY);
    y += 28;
    drawTextF(("Level: " + std::to_string(snap.levelIndex) + "/4").c_str(),
              lx, y, 30, LIGHTGRAY);
    y += 40;

    DrawLine(x + 13, y, x + w - 13, y, Color{80, 80, 100, 200});
    y += 18;

    const char* statNames[] = {"Academic", "Physical", "Mental", "Connection"};
    int stats[] = {snap.currentAcademic, snap.currentPhysical,
                   snap.currentMental, snap.currentConnection};
    int thresholds[] = {snap.thresholdAcademic, snap.thresholdPhysical,
                        snap.thresholdMental, snap.thresholdConnection};

    drawTextF("Player Stats", lx, y, 30, LIGHTGRAY);
    y += 40;

    for (int i = 0; i < 4; ++i) {
        Color ic = indicatorColor(i);
        drawTextF(statNames[i], lx, y, 26, ic);
        drawTextF(TextFormat("%d", stats[i]), lx + 150, y, 26, ic);

        int barX = lx + 194;
        int barW = 240;
        DrawRectangle(barX, y + 6, barW, 15, Color{50, 50, 50, 200});
        float pct = stats[i] / 100.0f;
        DrawRectangle(barX, y + 6, static_cast<int>(barW * pct), 15, ic);
        float tpct = thresholds[i] / 100.0f;
        int tx = barX + static_cast<int>(barW * tpct);
        DrawRectangle(tx - 1, y + 5, 4, 20, WHITE);

        y += 36;
    }

    y += 15;
    DrawLine(x + 13, y, x + w - 13, y, Color{80, 80, 100, 200});
    y += 18;

    bool canBuild = (snap.phase == GamePhase::Build ||
                     snap.phase == GamePhase::WaveCleared);

    if (canBuild) {
        drawTextF("Towers (click to select)", lx, y, 30, LIGHTGRAY);
        y += 42;

        TowerKind kinds[] = {TowerKind::Coffee, TowerKind::AI, TowerKind::Library,
                            TowerKind::Class, TowerKind::Bilibili};
        for (int i = 0; i < 5; ++i) {
            const TowerSpec spec = GameEngine::towerSpec(kinds[i]);
            Rectangle btn = {static_cast<float>(lx), static_cast<float>(y),
                             static_cast<float>(w - 39), 54.0f};
            bool hover = CheckCollisionPointRec(GetMousePosition(), btn);
            bool sel = (selectedTower == kinds[i]);

            Color bg = sel ? Color{60, 60, 100, 255}
                          : (hover ? Color{50, 50, 70, 255} : Color{40, 40, 55, 255});
            DrawRectangleRounded(btn, 0.3f, 8, bg);
            if (sel) DrawRectangleRoundedLines(btn, 0.3f, 8, 1.0f, WHITE);

            if (tm) {
                const char* sprite = towerSpriteName(spec.name);
                drawSprite(*tm, sprite, static_cast<float>(lx + 27), static_cast<float>(y + 27),
                           0.0224f, (gold >= spec.cost) ? WHITE : Color{100, 100, 100, 200});
                std::string label = std::string("  ") + spec.name + "  " + std::to_string(spec.cost) + "g";
                drawTextF(label.c_str(), lx + 45, y + 13, 26, (gold >= spec.cost) ? WHITE : GRAY);
            } else {
                Color tc = towerColor(spec.name);
                DrawCircle(lx + 24, y + 27, 10, tc);
                std::string label = std::string(spec.name) + "  " + std::to_string(spec.cost) + "g";
                drawTextF(label.c_str(), lx + 45, y + 13, 26, (gold >= spec.cost) ? tc : GRAY);
            }

            y += 60;
        }

        y += 15;
        DrawLine(x + 13, y, x + w - 13, y, Color{80, 80, 100, 200});
        y += 18;
    }

    if (canBuild) {
        Rectangle exBtn = {static_cast<float>(lx), static_cast<float>(y),
                           static_cast<float>(w - 39), 54.0f};
        bool exHover = CheckCollisionPointRec(GetMousePosition(), exBtn);
        DrawRectangleRounded(exBtn, 0.3f, 8,
                             exHover ? Color{60, 60, 80, 255} : Color{45, 45, 60, 255});
        drawTextF(exerciseMode ? "Exercise: ON" : "Exercise: OFF",
                  lx + 15, y + 13, 30, exerciseMode ? GREEN : GRAY);
        y += 64;
    }

    if (canBuild) {
        Rectangle swBtn = {static_cast<float>(lx), static_cast<float>(y),
                           static_cast<float>(w - 39), 72.0f};
        bool swHover = CheckCollisionPointRec(GetMousePosition(), swBtn);
        DrawRectangleRounded(swBtn, 0.4f, 8,
                             swHover ? Color{40, 130, 40, 255} : Color{30, 90, 30, 255});
        drawTextF("Start Wave", lx + 60, y + 15, 40, WHITE);
        y += 84;
    }

    y += 15;
    DrawLine(x + 13, y, x + w - 13, y, Color{80, 80, 100, 200});
    y += 18;

    if (canBuild) {
        if (showExerciseGuide) {
            drawExerciseGuide(lx, y, w - 39);
        } else {
            drawTowerGuide(selectedTower, lx, y, w - 39, selectedTowerIndex, tm);
        }
        y += 273;
    }

    if (exerciseMode) {
        drawTextF("Exercise: +1 Physical/3s", lx, y, 24, GREEN);
        y += 30;
        drawTextF("Tower DPS at 65%", lx, y, 24, Color{255, 200, 100, 255});
    }

    if (timeScale != 1.0f) {
        y += 30;
        char speedBuf[64];
        snprintf(speedBuf, sizeof(speedBuf), "Speed: %.1fx", timeScale);
        drawTextF(speedBuf, lx, y, 26, Color{255, 200, 50, 255});
    }
}

// ---- Main menu ----

void drawMainMenu(const TextureManager* tm) {
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                           Color{25, 25, 45, 255}, Color{15, 15, 28, 255});

    const char* title = "GPA Defender";
    int tw = measureTextF(title, 78);
    drawTextF(title, SCREEN_WIDTH / 2 - tw / 2, 195, 78, WHITE);

    const char* subtitle = "Tower Defense for Academic Survival";
    tw = measureTextF(subtitle, 34);
    drawTextF(subtitle, SCREEN_WIDTH / 2 - tw / 2, 299, 34, LIGHTGRAY);

    if (tm) {
        const char* menuChars[] = {"ai_coffee", "ai_ai",
                                    "ai_library", "ai_bilibili"};
        float charX[] = {754.0f, 858.0f, 1014.0f, 1196.0f};
        for (int i = 0; i < 4; ++i) {
            drawSprite(*tm, menuChars[i], charX[i], 494.0f, 0.08138f, WHITE);
        }
    }

    const char* prompt = "Press ENTER to Start";
    tw = measureTextF(prompt, 36);
    float alpha = 0.5f + 0.5f * sinf(static_cast<float>(GetTime()) * 3.0f);
    drawTextF(prompt, SCREEN_WIDTH / 2 - tw / 2, 663, 36,
              Color{255, 255, 255, static_cast<unsigned char>(alpha * 255)});

    const char* info = "Defend your GPA! Place towers, survive waves,";
    tw = measureTextF(info, 46);
    drawTextF(info, SCREEN_WIDTH / 2 - tw / 2, 767, 46, GRAY);

    info = "and keep all four indicators above threshold.";
    tw = measureTextF(info, 46);
    drawTextF(info, SCREEN_WIDTH / 2 - tw / 2, 829, 46, GRAY);
}

// ---- Level select ----

void drawLevelSelect(int unlockedLevel, int hoveredLevel, const TextureManager* tm) {
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                           Color{25, 25, 45, 255}, Color{15, 15, 28, 255});

    const char* title = "选择关卡";
    int tw = measureTextF(title, 64);
    drawTextF(title, SCREEN_WIDTH / 2 - tw / 2, 94, 64, WHITE);

    const char* subtitle = "完成关卡后会解锁下一关";
    tw = measureTextF(subtitle, 28);
    drawTextF(subtitle, SCREEN_WIDTH / 2 - tw / 2, 175, 28, LIGHTGRAY);

    const int cardW = 309;
    const int cardH = 500;
    const int gap = 70;
    const int totalW = cardW * 4 + gap * 3;
    const int startX = SCREEN_WIDTH / 2 - totalW / 2;
    const int cardY = 280;
    const char* levelNames[] = {"大一", "大二", "大三", "大四"};
    const char* levelChars[] = {"ai_coffee", "ai_library",
                                 "ai_ai", "ai_bilibili"};

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
        DrawRectangleRounded(
            {static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h)},
            0.2f, 8, bg);
        DrawRectangleRoundedLines(
            {static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h)},
            0.2f, 8, 2.0f, border);

        char levelBuf[32];
        snprintf(levelBuf, sizeof(levelBuf), "LEVEL %d", level);
        tw = measureTextF(levelBuf, 26);
        drawTextF(levelBuf, x + w / 2 - tw / 2, y + 55, 26,
                  unlocked ? Color{180, 190, 230, 255} : Color{95, 95, 110, 255});

        if (tm) {
            Color tint = unlocked ? WHITE : Color{80, 80, 80, 200};
            drawSprite(*tm, levelChars[i], static_cast<float>(x + w / 2), static_cast<float>(y + 230),
                       0.14f, tint);
        }

        tw = measureTextF(levelNames[i], 40);
        drawTextF(levelNames[i], x + w / 2 - tw / 2, y + 365, 40,
                  unlocked ? WHITE : Color{110, 110, 125, 255});

        const char* status = unlocked ? "已解锁" : "LOCKED";
        tw = measureTextF(status, unlocked ? 28 : 30);
        drawTextF(status, x + w / 2 - tw / 2, y + 435, unlocked ? 28 : 30,
                  unlocked ? Color{90, 220, 130, 255} : Color{210, 85, 85, 255});
    }

    Rectangle retryRect{
        SCREEN_WIDTH / 2.0f - 228.0f,
        1120.0f,
        455.0f,
        90.0f
    };
    bool retryHovered = CheckCollisionPointRec(GetMousePosition(), retryRect);
    Color retryBg = retryHovered ? Color{60, 60, 92, 255} : Color{38, 38, 58, 255};
    Color retryBorder = retryHovered ? Color{255, 220, 100, 255} : Color{90, 90, 130, 255};
    DrawRectangleRounded(retryRect, 0.2f, 8, retryBg);
    if (retryHovered) DrawRectangleRoundedLines(retryRect, 0.2f, 8, 1.0f, retryBorder);

    const char* retry = "Retry ASTI test";
    tw = measureTextF(retry, 30);
    drawTextF(retry,
              static_cast<int>(retryRect.x + retryRect.width / 2 - tw / 2),
              static_cast<int>(retryRect.y + 20),
              30,
              retryHovered ? WHITE : LIGHTGRAY);
}

// ---- Game over / Victory ----

void drawGameOver(int selection) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                  Color{0, 0, 0, 180});

    const char* msg = "GAME OVER";
    int tw = measureTextF(msg, 68);
    drawTextF(msg, SCREEN_WIDTH / 2 - tw / 2, 338, 68, Color{220, 60, 60, 255});

    const char* hint = "Your Score has fallen below threshold...";
    tw = measureTextF(hint, 23);
    drawTextF(hint, SCREEN_WIDTH / 2 - tw / 2, 442, 23, LIGHTGRAY);

    const char* retry = "Retry This Level";
    const char* menu = "Return to Main Menu";

    Color retryColor = (selection == 0) ? Color{255, 220, 100, 255} : GRAY;
    Color menuColor  = (selection == 1) ? Color{255, 220, 100, 255} : GRAY;

    tw = measureTextF(retry, 26);
    drawTextF(retry, SCREEN_WIDTH / 2 - tw / 2, 520, 26, retryColor);

    tw = measureTextF(menu, 26);
    drawTextF(menu, SCREEN_WIDTH / 2 - tw / 2, 572, 26, menuColor);

    hint = "Use UP/DOWN to select, ENTER to confirm";
    tw = measureTextF(hint, 18);
    drawTextF(hint, SCREEN_WIDTH / 2 - tw / 2, 650, 18, DARKGRAY);
}

void drawVictory(int selection, bool hasNextLevel) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                  Color{0, 0, 0, 180});

    const char* msg = "VICTORY!";
    int tw = measureTextF(msg, 68);
    drawTextF(msg, SCREEN_WIDTH / 2 - tw / 2, 338, 68, Color{80, 220, 80, 255});

    const char* hint = "You have successfully defended your GPA!";
    tw = measureTextF(hint, 23);
    drawTextF(hint, SCREEN_WIDTH / 2 - tw / 2, 442, 23, LIGHTGRAY);

    const char* nextLevel = "Continue to Next Level";
    const char* menu = "Return to Main Menu";

    if (hasNextLevel) {
        Color nextColor = (selection == 0) ? Color{255, 220, 100, 255} : GRAY;
        Color menuColor = (selection == 1) ? Color{255, 220, 100, 255} : GRAY;

        tw = measureTextF(nextLevel, 26);
        drawTextF(nextLevel, SCREEN_WIDTH / 2 - tw / 2, 520, 26, nextColor);

        tw = measureTextF(menu, 26);
        drawTextF(menu, SCREEN_WIDTH / 2 - tw / 2, 572, 26, menuColor);
    } else {
        Color menuColor = (selection == 0) ? Color{255, 220, 100, 255} : GRAY;
        tw = measureTextF(menu, 26);
        drawTextF(menu, SCREEN_WIDTH / 2 - tw / 2, 520, 26, menuColor);
    }

    hint = "Use UP/DOWN to select, ENTER to confirm";
    tw = measureTextF(hint, 18);
    drawTextF(hint, SCREEN_WIDTH / 2 - tw / 2, 650, 18, DARKGRAY);
}

// ---- Questionnaire / ASTI ----

void drawQuestionnaire(const Questionnaire& q, int current,
                       const std::vector<int>& answers) {
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                           Color{25, 25, 45, 255}, Color{15, 15, 28, 255});

    const auto& questions = q.getQuestions();
    if (current < 0 || current >= static_cast<int>(questions.size())) return;

    const auto& question = questions[current];

    const char* title = "ASTI Personality Quiz";
    int tw = measureTextF(title, 57);
    drawTextF(title, SCREEN_WIDTH / 2 - tw / 2, 47, 57, WHITE);

    std::string progress = "Question " + std::to_string(current + 1) + " / " +
                           std::to_string(static_cast<int>(questions.size()));
    tw = measureTextF(progress.c_str(), 50);
    drawTextF(progress.c_str(), SCREEN_WIDTH / 2 - tw / 2, 135, 50, GRAY);

    DrawRectangleRounded(
        {78.0f, 195.0f, static_cast<float>(SCREEN_WIDTH - 156), 170.0f}, 0.2f, 8,
        Color{30, 30, 50, 255});
    DrawRectangleRoundedLines(
        {78.0f, 195.0f, static_cast<float>(SCREEN_WIDTH - 156), 170.0f}, 0.2f, 8, 1.0f,
        Color{80, 80, 120, 255});
    drawTextF(question.prompt.c_str(), 104, 233, 55, WHITE);

    for (size_t i = 0; i < question.options.size(); ++i) {
        const int optionHeight = 170;
        const int optionStep = 195;
        int oy = 400 + static_cast<int>(i) * optionStep;
        bool selected = (current < static_cast<int>(answers.size()) &&
                         answers[current] == static_cast<int>(i));
        Color bg = selected ? Color{50, 50, 100, 255} : Color{35, 35, 55, 255};
        DrawRectangleRounded(
            {130.0f, static_cast<float>(oy), static_cast<float>(SCREEN_WIDTH - 260), static_cast<float>(optionHeight)},
            0.2f, 8, bg);
        DrawRectangleRoundedLines(
            {130.0f, static_cast<float>(oy), static_cast<float>(SCREEN_WIDTH - 260), static_cast<float>(optionHeight)},
            0.2f, 8, 1.0f, selected ? WHITE : Color{80, 80, 120, 255});

        std::string label = std::to_string(i + 1) + ".";
        drawTextF(label.c_str(), 156, oy + 16, 48, LIGHTGRAY);

        drawTextF(question.options[i].text.c_str(), 195, oy + 14, 48,
                  selected ? WHITE : LIGHTGRAY);

        std::string effStr;
        for (const auto& eff : question.options[i].effects) {
            const char* indName[] = {"Academic", "Physical", "Mental", "Connection"};
            if (!effStr.empty()) effStr += ", ";
            char sign = eff.delta >= 0 ? '+' : '-';
            effStr += std::string(indName[static_cast<int>(eff.indicator)]) +
                      " " + sign + std::to_string(abs(eff.delta));
        }
        drawTextF(effStr.c_str(), 195, oy + optionHeight - 56, 48, GRAY);
    }

    const char* hint = "Press 1-4 to select an answer";
    tw = measureTextF(hint, 52);
    drawTextF(hint, SCREEN_WIDTH / 2 - tw / 2, SCREEN_HEIGHT - 80, 52, GRAY);
}

void drawAstiSummary(const AstiResult& result) {
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                           Color{25, 25, 45, 255}, Color{15, 15, 28, 255});

    const char* title = "你的 ASTI 类型";
    int tw = measureTextF(title, 65);
    drawTextF(title, SCREEN_WIDTH / 2 - tw / 2, 94, 65, WHITE);

    const char* subtitle = "Attend School Type Indicator 上学人格。";
    tw = measureTextF(subtitle, 36);
    drawTextF(subtitle, SCREEN_WIDTH / 2 - tw / 2, 174, 36, LIGHTGRAY);

    DrawRectangleRounded(
        {156.0f, 260.0f, static_cast<float>(SCREEN_WIDTH - 312), 195.0f}, 0.2f, 8,
        Color{35, 35, 58, 255});
    DrawRectangleRoundedLines(
        {156.0f, 260.0f, static_cast<float>(SCREEN_WIDTH - 312), 195.0f}, 0.2f, 8, 1.0f,
        Color{90, 90, 135, 255});

    drawTextF("结果类型", 202, 296, 39, WHITE);

    std::string tagText;
    for (size_t i = 0; i < result.tags.size(); ++i) {
        if (!tagText.empty()) tagText += " / ";
        tagText += result.tags[i];
    }
    if (tagText.empty()) tagText = "未识别";

    tw = measureTextF(tagText.c_str(), 57);
    drawTextF(tagText.c_str(), SCREEN_WIDTH / 2 - tw / 2, 354, 57, GOLD);

    const int cardX = 195;
    const int cardY = 533;
    const int cardW = SCREEN_WIDTH - 390;
    const int rowH = 81;

    drawTextF("最终阈值分数", cardX, 486, 36, WHITE);
    drawTextF("被怪兽攻击至该分数以下则失败", cardX + 234, 491, 29, LIGHTGRAY);

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
        DrawRectangleRounded(
            {static_cast<float>(cardX), static_cast<float>(y),
             static_cast<float>(cardW), static_cast<float>(rowH - 10)},
            0.15f, 8, (i % 2 == 0) ? Color{32, 32, 50, 255} : Color{38, 38, 58, 255});
        DrawRectangleRoundedLines(
            {static_cast<float>(cardX), static_cast<float>(y),
             static_cast<float>(cardW), static_cast<float>(rowH - 10)},
            0.15f, 8, 1.0f, Color{75, 75, 105, 210});
        DrawCircle(cardX + 36, y + 35, 12, c);
        drawTextF(names[i], cardX + 68, y + 21, 31, LIGHTGRAY);

        std::string valueStr = std::to_string(values[i]);
        int valueW = measureTextF(valueStr.c_str(), 36);
        drawTextF(valueStr.c_str(), cardX + cardW - valueW - 44, y + 18, 36, c);
    }

    const char* hint = "按 Enter 继续进入游戏";
    tw = measureTextF(hint, 26);
    float alpha = 0.5f + 0.5f * sinf(static_cast<float>(GetTime()) * 3.0f);
    drawTextF(hint, SCREEN_WIDTH / 2 - tw / 2, SCREEN_HEIGHT - 107, 26,
              Color{255, 255, 255, static_cast<unsigned char>(alpha * 255)});
}

}  // namespace frontend
