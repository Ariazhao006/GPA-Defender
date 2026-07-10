#include "frontend/Renderer.h"
#include "frontend/TextureManager.h"

#include "gpa_defender/PlayerStats.h"
#include "raylib.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace frontend {
namespace {

constexpr Color kPaper = {246, 248, 244, 255};
constexpr Color kPanel = {255, 255, 252, 248};
constexpr Color kPanelLine = {214, 220, 211, 255};
constexpr Color kInk = {32, 41, 38, 255};
constexpr Color kMuted = {78, 91, 84, 255};
constexpr Color kSoft = {235, 240, 234, 255};
constexpr Color kAccent = {42, 116, 86, 255};
constexpr Color kAccentSoft = {219, 239, 228, 255};
constexpr Color kWarn = {196, 133, 40, 255};

} // namespace

static Font gUiFont;
static RenderTexture2D gVirtualCanvas{};
static bool gVirtualCanvasReady = false;
static float gVirtualScale = 1.0f;
static Vector2 gVirtualOffset{0.0f, 0.0f};

void setUiFont(Font font) { gUiFont = font; }
const Font& getUiFont() { return gUiFont; }

void initVirtualCanvas() {
    if (gVirtualCanvasReady) return;
    gVirtualCanvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTextureFilter(gVirtualCanvas.texture, TEXTURE_FILTER_BILINEAR);
    gVirtualCanvasReady = true;
}

void shutdownVirtualCanvas() {
    if (!gVirtualCanvasReady) return;
    UnloadRenderTexture(gVirtualCanvas);
    gVirtualCanvas = {};
    gVirtualCanvasReady = false;
}

void beginVirtualDrawing() {
    if (!gVirtualCanvasReady) initVirtualCanvas();

    const float scaleX = GetScreenWidth() / static_cast<float>(SCREEN_WIDTH);
    const float scaleY = GetScreenHeight() / static_cast<float>(SCREEN_HEIGHT);
    gVirtualScale = std::max(0.01f, std::min(scaleX, scaleY));
    const float drawW = SCREEN_WIDTH * gVirtualScale;
    const float drawH = SCREEN_HEIGHT * gVirtualScale;
    gVirtualOffset = {
        (GetScreenWidth() - drawW) * 0.5f,
        (GetScreenHeight() - drawH) * 0.5f
    };

    SetMouseOffset(static_cast<int>(-gVirtualOffset.x), static_cast<int>(-gVirtualOffset.y));
    SetMouseScale(1.0f / gVirtualScale, 1.0f / gVirtualScale);

    BeginTextureMode(gVirtualCanvas);
}

void endVirtualDrawing() {
    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(gVirtualCanvas.texture,
                   {0.0f, 0.0f,
                    static_cast<float>(gVirtualCanvas.texture.width),
                    static_cast<float>(-gVirtualCanvas.texture.height)},
                   {gVirtualOffset.x, gVirtualOffset.y,
                    SCREEN_WIDTH * gVirtualScale,
                    SCREEN_HEIGHT * gVirtualScale},
                   {0.0f, 0.0f}, 0.0f, WHITE);
    EndDrawing();
}

Rectangle mainMenuStartRect() {
    return {SCREEN_WIDTH / 2.0f - 200.0f, 630.0f, 400.0f, 76.0f};
}

Rectangle mainMenuContinueRect() {
    return {SCREEN_WIDTH / 2.0f - 300.0f, 620.0f, 280.0f, 76.0f};
}

Rectangle mainMenuNewGameRect(bool hasSavedGame) {
    if (hasSavedGame) {
        return {SCREEN_WIDTH / 2.0f + 20.0f, 620.0f, 280.0f, 76.0f};
    }
    return mainMenuStartRect();
}

Rectangle mainMenuSaveSlotsRect() {
    return {SCREEN_WIDTH / 2.0f - 200.0f, 720.0f, 400.0f, 66.0f};
}

Rectangle questionnaireOptionRect(std::size_t index) {
    return {130.0f,
            400.0f + static_cast<float>(index) * 195.0f,
            static_cast<float>(SCREEN_WIDTH - 260),
            170.0f};
}

Rectangle astiContinueRect() {
    return {SCREEN_WIDTH / 2.0f - 220.0f,
            SCREEN_HEIGHT - 145.0f,
            440.0f,
            72.0f};
}

Rectangle gameOverOptionRect(int option) {
    return {SCREEN_WIDTH / 2.0f - 240.0f,
            500.0f + option * 60.0f,
            480.0f,
            48.0f};
}

Rectangle victoryOptionRect(int option, bool hasNextLevel) {
    if (hasNextLevel) {
        const float y = 500.0f + option * 60.0f;
        return {SCREEN_WIDTH / 2.0f - 255.0f, y, 510.0f, 48.0f};
    }
    const float y = 1130.0f;
    if (option == 0) return {775.0f, y, 320.0f, 64.0f};
    return {1135.0f, y, 320.0f, 64.0f};
}

Rectangle seedBarTowerRect(int index) {
    return {220.0f + index * 160.0f, 18.0f, 142.0f, 126.0f};
}

Rectangle levelSelectRetakeRect() {
    return {SCREEN_WIDTH / 2.0f - 480.0f, 1120.0f, 430.0f, 90.0f};
}

Rectangle levelSelectReturnRect() {
    return {SCREEN_WIDTH / 2.0f + 50.0f, 1120.0f, 430.0f, 90.0f};
}

Rectangle gambleSkipRect() {
    return {SCREEN_WIDTH / 2.0f - 445.0f, SCREEN_HEIGHT / 2.0f + 54.0f, 365.0f, 230.0f};
}

Rectangle gambleNoSkipRect() {
    return {SCREEN_WIDTH / 2.0f + 80.0f, SCREEN_HEIGHT / 2.0f + 54.0f, 365.0f, 230.0f};
}

Rectangle gambleResultContinueRect() {
    return {SCREEN_WIDTH / 2.0f - 180.0f, SCREEN_HEIGHT / 2.0f + 244.0f, 360.0f, 64.0f};
}

int measureTextF(const char* text, int fontSize) {
    Vector2 sz = MeasureTextEx(gUiFont, text, static_cast<float>(fontSize), 0.0f);
    return static_cast<int>(sz.x);
}

void drawTextF(const char* text, int x, int y, int fontSize, Color color) {
    Vector2 pos = {static_cast<float>(x), static_cast<float>(y)};
    DrawTextEx(gUiFont, text,
               pos,
               static_cast<float>(fontSize), 0.0f, color);
    DrawTextEx(gUiFont, text,
               {pos.x + 0.65f, pos.y},
               static_cast<float>(fontSize), 0.0f, color);
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

Color towerRangeColor(const std::string& name) {
    if (name.find("Coffee") != std::string::npos)   return Color{197, 121, 48, 255};
    if (name.find("AI") != std::string::npos)        return Color{53, 142, 214, 255};
    if (name.find("Library") != std::string::npos)   return Color{74, 103, 205, 255};
    if (name.find("Class") != std::string::npos)     return Color{219, 142, 45, 255};
    if (name.find("Bilibili") != std::string::npos)  return Color{222, 82, 152, 255};
    return Color{80, 160, 120, 255};
}

Color withAlpha(Color color, unsigned char alpha) {
    color.a = alpha;
    return color;
}

Color enemyColor(const std::string& name) {
    if (name.find("Midterm") != std::string::npos)
        return Color{180, 30, 30, 255};
    if (name.find("Calculus") != std::string::npos)
        return RED;
    if (name.find("Research") != std::string::npos)
        return Color{100, 0, 100, 255};
    if (name.find("Friends") != std::string::npos)
        return GOLD;
    if (name.find("Morning") != std::string::npos)
        return ORANGE;
    if (name.find("Group") != std::string::npos)
        return GRAY;
    if (name.find("Video") != std::string::npos)
        return MAGENTA;
    if (name.find("Syllabus") != std::string::npos)
        return LIME;
    if (name.find("Pressure") != std::string::npos)
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

unsigned int tileHash(int row, int col) {
    unsigned int x = static_cast<unsigned int>(row * 374761393u + col * 668265263u);
    x = (x ^ (x >> 13u)) * 1274126177u;
    return x ^ (x >> 16u);
}

const Texture2D* highlandTileTexture(const TextureManager& tm, int row, int col) {
    constexpr int kClusterSize = 2;
    const int clusterRow = row / kClusterSize;
    const int clusterCol = col / kClusterSize;

    // mapTile_017 is the main highland tile; mapTile_015 appears as clustered accents.
    if (tileHash(clusterRow, clusterCol) % 100u < 20u) {
        return tm.highlandTile015.id != 0 ? &tm.highlandTile015 : nullptr;
    }
    return tm.highlandTile017.id != 0 ? &tm.highlandTile017 : nullptr;
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
    if (enemyName.find("Midterm") != std::string::npos)
        return "ai_boss";
    if (enemyName.find("Calculus") != std::string::npos)
        return "ai_calculus";
    if (enemyName.find("Research") != std::string::npos)
        return "ai_research";
    if (enemyName.find("Friends") != std::string::npos)
        return "ai_social";
    if (enemyName.find("Morning") != std::string::npos)
        return "ai_morning";
    if (enemyName.find("Group") != std::string::npos)
        return "ai_group";
    if (enemyName.find("Video") != std::string::npos)
        return "ai_video";
    if (enemyName.find("Syllabus") != std::string::npos)
        return "ai_exam_outline";
    if (enemyName.find("Pressure") != std::string::npos)
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

            if (tm) {
                Rectangle src;
                if (type == TileType::Highland) {
                    const Texture2D* highlandTexture = highlandTileTexture(*tm, row, col);
                    if (highlandTexture) {
                        DrawTexturePro(*highlandTexture,
                                       {0.0f, 0.0f,
                                        static_cast<float>(highlandTexture->width),
                                        static_cast<float>(highlandTexture->height)},
                                       {x, y, static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)},
                                       {0, 0}, 0.0f, WHITE);
                    } else {
                        DrawRectangle(static_cast<int>(x), static_cast<int>(y),
                                      TILE_SIZE, TILE_SIZE, tileColor(type));
                    }
                } else if (type == TileType::Path && tm->pathTile.id != 0) {
                    DrawTexturePro(tm->pathTile,
                                   {0.0f, 0.0f,
                                    static_cast<float>(tm->pathTile.width),
                                    static_cast<float>(tm->pathTile.height)},
                                   {x, y, static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)},
                                   {0, 0}, 0.0f, WHITE);
                } else if (type == TileType::Spawn && tm->spawnTile.id != 0) {
                    DrawRectangle(static_cast<int>(x), static_cast<int>(y),
                                  TILE_SIZE, TILE_SIZE, Color{238, 243, 236, 255});
                    DrawTexturePro(tm->spawnTile,
                                   {0.0f, 0.0f,
                                    static_cast<float>(tm->spawnTile.width),
                                    static_cast<float>(tm->spawnTile.height)},
                                   {x + 13.0f, y + 13.0f,
                                    static_cast<float>(TILE_SIZE - 26),
                                    static_cast<float>(TILE_SIZE - 26)},
                                   {0, 0}, 0.0f, WHITE);
                } else if (type == TileType::Base) {
                    DrawRectangle(static_cast<int>(x), static_cast<int>(y),
                                  TILE_SIZE, TILE_SIZE, Color{244, 247, 242, 255});
                    DrawRectangleRounded({x + 8.0f, y + 8.0f,
                                          static_cast<float>(TILE_SIZE - 16),
                                          static_cast<float>(TILE_SIZE - 16)},
                                         0.18f, 8, Color{232, 239, 232, 255});
                } else {
                    if (tm->mapTilesheet.id != 0) {
                        switch (type) {
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
                        DrawRectangle(static_cast<int>(x), static_cast<int>(y),
                                      TILE_SIZE, TILE_SIZE, tileColor(type));
                    }
                }
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

        }
    }
}

void drawBaseHealth(const Block& block, int hp, int maxHp) {
    if (maxHp <= 0) return;

    const auto& grid = block.getGrid();
    for (int row = 0; row < static_cast<int>(grid.size()); ++row) {
        for (int col = 0; col < static_cast<int>(grid[row].size()); ++col) {
            if (grid[row][col].type != TileType::Base) continue;

            const float x = static_cast<float>(MAP_OFFSET_X + col * TILE_SIZE);
            const float y = static_cast<float>(MAP_OFFSET_Y + row * TILE_SIZE);
            const float pulse = 0.5f + 0.5f * sinf(static_cast<float>(GetTime()) * 3.0f);
            const float ratio = std::max(0.0f, std::min(1.0f, hp / static_cast<float>(maxHp)));
            const float gpa = ratio * 4.0f;
            const float barW = static_cast<float>(TILE_SIZE - 18);
            const float barH = 11.0f;
            const float barX = x + 9.0f;
            const float barY = y + 73.0f;

            DrawCircleLines(static_cast<int>(x + TILE_SIZE / 2),
                            static_cast<int>(y + TILE_SIZE / 2),
                            36.0f + pulse * 3.0f,
                            Color{42, 116, 86, static_cast<unsigned char>(85 + pulse * 75)});
            DrawRectangle(static_cast<int>(barX), static_cast<int>(barY),
                          static_cast<int>(barW), static_cast<int>(barH),
                          Color{211, 219, 211, 255});
            DrawRectangle(static_cast<int>(barX), static_cast<int>(barY),
                          static_cast<int>(barW * ratio), static_cast<int>(barH),
                          ratio > 0.5f ? kAccent : (ratio > 0.25f ? kWarn : RED));
            DrawRectangleLines(static_cast<int>(barX), static_cast<int>(barY),
                               static_cast<int>(barW), static_cast<int>(barH), kPanelLine);

            char gpaBuf[32];
            snprintf(gpaBuf, sizeof(gpaBuf), "GPA\n%.1f", gpa);
            std::vector<std::string> lines = splitTextLines(gpaBuf);
            const int tw = measureTextF(lines[0].c_str(), 22);
            const int vw = measureTextF(lines[1].c_str(), 30);
            drawTextF(lines[0].c_str(), static_cast<int>(x + TILE_SIZE / 2 - tw / 2),
                      static_cast<int>(y + 25), 22, kMuted);
            drawTextF(lines[1].c_str(), static_cast<int>(x + TILE_SIZE / 2 - vw / 2),
                      static_cast<int>(y + 45), 30, kAccent);
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
        Color rangeColor = towerRangeColor(tower.getName());
        DrawCircle(static_cast<int>(center.x), static_cast<int>(center.y),
                   tower.getRange(), withAlpha(rangeColor, 24));
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y),
                        tower.getRange(), withAlpha(rangeColor, 170));
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y),
                        tower.getRange() + 2.0f, withAlpha(rangeColor, 95));
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

    int fontSize = 18;
    const char* name = tower.getName().c_str();
    int tw = measureTextF(name, fontSize);
    drawTextF(name, static_cast<int>(center.x - tw / 2),
              static_cast<int>(center.y + 31), fontSize, kInk);
}

void drawTowers(const std::vector<std::unique_ptr<DefenseTower>>& towers,
                int selectedIndex, const TextureManager* tm) {
    std::vector<std::size_t> order;
    order.reserve(towers.size());
    for (std::size_t i = 0; i < towers.size(); ++i) {
        order.push_back(i);
    }
    std::sort(order.begin(), order.end(),
        [&](std::size_t a, std::size_t b) {
            const Vector2D pa = towers[a]->getPosition();
            const Vector2D pb = towers[b]->getPosition();
            if (pa.y == pb.y) return pa.x < pb.x;
            return pa.y < pb.y;
        });

    for (std::size_t i : order) {
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
    const float bob = sinf(static_cast<float>(GetTime()) * 3.4f + center.x * 0.021f) * 4.0f;
    Vector2 renderCenter = {center.x, center.y + bob};

    float scale = 0.08385f;

    if (tm) {
        const char* sprite = enemySpriteName(enemy.getName());
        drawSprite(*tm, "shadow", center.x, center.y + 8, scale * 0.9f, Color{0, 0, 0, 120});
        drawSprite(*tm, sprite, renderCenter.x, renderCenter.y - 26, scale, WHITE);
    } else {
        Color c = enemyColor(enemy.getName());
        float radius = 28.6f;
        DrawCircle(static_cast<int>(renderCenter.x), static_cast<int>(renderCenter.y), radius, c);
        DrawCircleLines(static_cast<int>(renderCenter.x), static_cast<int>(renderCenter.y),
                        radius, Color{0, 0, 0, 180});
    }

    // HP bar
    float hpPct = static_cast<float>(enemy.getHp()) / enemy.getMaxHp();
    float barW = 65.0f;
    float barH = 5.0f;
    float barX = center.x - barW / 2.0f;
    float barY = renderCenter.y - 96.0f;

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
            drawSprite(*tm, "chest_case", static_cast<float>(x), static_cast<float>(y),
                       0.052f, cc);
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

        if (chest.target) {
            const int hp = chest.target->getHp();
            const int maxHp = chest.target->getMaxHp();
            const float ratio = maxHp > 0 ? hp / static_cast<float>(maxHp) : 0.0f;
            const int barW = 60;
            const int barH = 6;
            const int barX = x - barW / 2;
            const int barY = y - 48;
            DrawRectangle(barX, barY, barW, barH, Color{40, 40, 40, 220});
            DrawRectangle(barX, barY, static_cast<int>(barW * ratio), barH, GOLD);
            DrawRectangleLines(barX, barY, barW, barH, Color{255, 255, 255, 180});

            const char* label = chestLabel(chest.type);
            int tw = measureTextF(label, 18);
            drawTextF(label, x - tw / 2, y - 72, 18, WHITE);
        }

        if (!chest.armedForAttack) {
            const char* hint = "CLICK";
            int tw = measureTextF(hint, 18);
            drawTextF(hint, x - tw / 2, y + 31, 18, kInk);
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
    const char* title = "Tower Guide";
    const char* name = "Coffee";
    std::vector<std::string> lines;

    switch (selectedTower) {
    case TowerKind::Coffee:
        name = "Coffee";
        lines = {"Short range, high burst.", "Targets the nearest enemy.", "Best near tight corners."};
        break;
    case TowerKind::AI:
        name = "AI";
        lines = {"Hits every enemy in range.", "Upgrades improve damage, range,", "and attack speed."};
        break;
    case TowerKind::Library:
        name = "Library";
        lines = {"No direct damage.", "Slows enemies in range.", "Pairs well with damage towers."};
        break;
    case TowerKind::Class:
        name = "Class";
        lines = {"Heavy single hit.", "Long cooldown.", "Holds key choke points."};
        break;
    case TowerKind::Bilibili:
        name = "Bilibili";
        lines = {"Straight-line barrage.", "Press R before placement to rotate.", "Select it and press R to re-aim."};
        break;
    }

    DrawRectangleRounded(
        {static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), 255.0f},
        0.12f, 8, Color{250, 251, 248, 245});
    DrawRectangleRoundedLines(
        {static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), 255.0f},
        0.12f, 8, 1.0f, kPanelLine);

    if (tm) {
        const char* sprite = towerSpriteName(name);
        drawSprite(*tm, sprite, static_cast<float>(x + 33), static_cast<float>(y + 39), 0.024f, WHITE);
    } else {
        Color tc = towerColor(name);
        DrawCircle(x + 24, y + 30, 12, tc);
    }
    drawTextF(title, x + 55, y + 18, 30, kInk);
    drawTextF(name, x + width - measureTextF(name, 30) - 20, y + 18, 30, towerColor(name));

    int lineY = y + 76;
    for (const std::string& line : lines) {
        drawTextF(line.c_str(), x + 20, lineY, 24, kMuted);
        lineY += 42;
    }

    if (selectedTowerIndex < 0) {
        drawTextF("Click a highland tile to place it.", x + 20, y + 220, 20, kMuted);
    } else {
        drawTextF("A placed tower is selected.", x + 20, y + 220, 20, kMuted);
    }
}

void drawExerciseGuide(int x, int y, int width) {
    DrawRectangleRounded(
        {static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), 255.0f},
        0.12f, 8, Color{250, 251, 248, 245});
    DrawRectangleRoundedLines(
        {static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), 255.0f},
        0.12f, 8, 1.0f, kPanelLine);

    DrawCircle(x + 24, y + 33, 12, GREEN);
    drawTextF("Exercise Mode", x + 50, y + 18, 30, kInk);
    drawTextF("Exercise", x + width - measureTextF("Exercise", 30) - 20,
              y + 18, 30, GREEN);

    std::vector<std::string> lines = {
        "During waves, restores GPA",
        "by 0.1 every 5 seconds.",
        "Tower fire rate drops to 65%."
    };

    int lineY = y + 76;
    for (const std::string& line : lines) {
        drawTextF(line.c_str(), x + 20, lineY, 24, kMuted);
        lineY += 42;
    }

    drawTextF("Click Exercise again to turn it off.", x + 20, y + 220, 20, kMuted);
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

    Color rangeColor = towerRangeColor(spec.name);
    DrawCircle(static_cast<int>(center.x), static_cast<int>(center.y),
               spec.range, withAlpha(rangeColor, 20));
    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y),
                    spec.range, withAlpha(rangeColor, 150));

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

void drawSeedBar(int gold, TowerKind selectedTower, const TextureManager* tm) {
    DrawRectangle(0, 0, UI_PANEL_X, MAP_OFFSET_Y - 35, Color{250, 251, 248, 250});
    DrawRectangle(0, MAP_OFFSET_Y - 36, UI_PANEL_X, 1, kPanelLine);

    Rectangle goldCard = {18.0f, 18.0f, 170.0f, 126.0f};
    DrawRectangleRounded(goldCard, 0.08f, 8, Color{255, 255, 252, 255});
    DrawRectangleRoundedLines(goldCard, 0.08f, 8, 1.5f, kPanelLine);
    if (tm) {
        drawSprite(*tm, "tile_coin", goldCard.x + 44.0f, goldCard.y + 52.0f, 0.78f, GOLD);
    } else {
        DrawCircle(static_cast<int>(goldCard.x + 42), static_cast<int>(goldCard.y + 50), 16, GOLD);
    }
    drawTextF(std::to_string(gold).c_str(), static_cast<int>(goldCard.x + 75),
              static_cast<int>(goldCard.y + 34), 40, kWarn);
    drawTextF("FOCUS", static_cast<int>(goldCard.x + 44),
              static_cast<int>(goldCard.y + 86), 20, kMuted);

    TowerKind kinds[] = {TowerKind::Coffee, TowerKind::AI, TowerKind::Library,
                         TowerKind::Class, TowerKind::Bilibili};

    for (int i = 0; i < 5; ++i) {
        const TowerSpec spec = GameEngine::towerSpec(kinds[i]);
        Rectangle card = seedBarTowerRect(i);
        bool selected = selectedTower == kinds[i];
        bool affordable = gold >= spec.cost;
        bool hover = CheckCollisionPointRec(GetMousePosition(), card);
        Color cardBg = selected ? kAccentSoft
                      : (hover ? Color{245, 248, 244, 255} : Color{255, 255, 252, 255});
        Color border = selected ? kAccent : (hover ? Color{160, 180, 170, 255} : kPanelLine);

        DrawRectangleRounded(card, 0.08f, 8, cardBg);
        DrawRectangleRoundedLines(card, 0.08f, 8, selected ? 2.0f : 1.3f, border);

        Rectangle pricePill = {card.x + 10.0f, card.y + 9.0f, 52.0f, 28.0f};
        DrawRectangleRounded(pricePill, 0.4f, 8,
                             affordable ? Color{255, 246, 199, 255} : Color{235, 238, 235, 255});
        drawTextF(std::to_string(spec.cost).c_str(), static_cast<int>(pricePill.x + 9),
                  static_cast<int>(pricePill.y + 3), 18, affordable ? kWarn : kMuted);

        Color iconBg = affordable ? Color{235, 242, 246, 255} : Color{235, 238, 235, 255};
        DrawCircle(static_cast<int>(card.x + card.width / 2),
                   static_cast<int>(card.y + 57), 30, iconBg);
        if (tm) {
            drawSprite(*tm, towerSpriteName(spec.name),
                       card.x + card.width / 2, card.y + 51.0f, 0.031f,
                       affordable ? WHITE : Color{145, 150, 145, 190});
        } else {
            DrawCircle(static_cast<int>(card.x + card.width / 2),
                       static_cast<int>(card.y + 57), 13,
                       affordable ? towerColor(spec.name) : kMuted);
        }

        std::string label = spec.name;
        if (label == "AI") label = "AI Assistant";
        int labelW = measureTextF(label.c_str(), 18);
        drawTextF(label.c_str(), static_cast<int>(card.x + card.width / 2 - labelW / 2),
                  static_cast<int>(card.y + 100), 18, affordable ? kInk : kMuted);
    }
}

void drawUI(const GameSnapshot& snap, int gold, int currentLevel, TowerKind selectedTower,
            bool exerciseMode, int selectedTowerIndex, bool showExerciseGuide,
            float timeScale, float panelScrollOffset, const TextureManager* tm) {
    int x = UI_PANEL_X;
    int w = UI_PANEL_WIDTH;

    DrawRectangle(x, 0, w, SCREEN_HEIGHT, kPanel);
    DrawRectangleLines(x, 0, w, SCREEN_HEIGHT, kPanelLine);

    BeginScissorMode(x, 0, w, SCREEN_HEIGHT);

    int y = 26 - static_cast<int>(panelScrollOffset);
    const int lx = x + 20;

    drawTextF(GameEngine::phaseName(snap.phase), lx, y, 42, phaseColor(snap.phase));
    y += 66;

    // Gold with coin icon
    if (tm) {
        drawSprite(*tm, "tile_coin", static_cast<float>(lx + 15), static_cast<float>(y + 15),
                   0.60f, GOLD);
        drawTextF(("  " + std::to_string(gold)).c_str(), lx + 27, y, 40, kWarn);
    } else {
        drawTextF(("Gold: " + std::to_string(gold)).c_str(), lx, y, 40, kWarn);
    }
    y += 48;
    drawTextF(("Wave: " + std::to_string(snap.waveIndex + 1) + "/3").c_str(),
              lx, y, 30, kMuted);
    y += 40;
    drawTextF(("Level: " + std::to_string(currentLevel) + "/4").c_str(),
              lx, y, 30, kMuted);
    y += 50;

    DrawLine(x + 13, y, x + w - 13, y, kPanelLine);
    y += 18;

    const char* statNames[] = {"Academic", "Physical", "Mental", "Connection"};
    int stats[] = {snap.currentAcademic, snap.currentPhysical,
                   snap.currentMental, snap.currentConnection};
    int thresholds[] = {snap.thresholdAcademic, snap.thresholdPhysical,
                        snap.thresholdMental, snap.thresholdConnection};

    drawTextF("Player Stats", lx, y, 30, kInk);
    y += 40;

    for (int i = 0; i < 4; ++i) {
        Color ic = indicatorColor(i);
        drawTextF(statNames[i], lx, y, 26, ic);
        drawTextF(TextFormat("%d", stats[i]), lx + 150, y, 26, ic);

        int barX = lx + 194;
        int barW = 240;
        DrawRectangle(barX, y + 6, barW, 15, Color{226, 231, 226, 255});
        float pct = stats[i] / 100.0f;
        DrawRectangle(barX, y + 6, static_cast<int>(barW * pct), 15, ic);
        float tpct = thresholds[i] / 100.0f;
        int tx = barX + static_cast<int>(barW * tpct);
        DrawRectangle(tx - 1, y + 5, 4, 20, kInk);

        y += 36;
    }

    y += 15;
    DrawLine(x + 13, y, x + w - 13, y, kPanelLine);
    y += 18;

    bool canBuild = (snap.phase == GamePhase::Build ||
                     snap.phase == GamePhase::WaveRunning ||
                     snap.phase == GamePhase::WaveCleared);

    if (canBuild) {
        Rectangle exBtn = {static_cast<float>(lx), static_cast<float>(y),
                           static_cast<float>(w - 39), 54.0f};
        bool exHover = CheckCollisionPointRec(GetMousePosition(), exBtn);
        DrawRectangleRounded(exBtn, 0.3f, 8,
                             exHover ? Color{235, 244, 236, 255} : Color{250, 251, 248, 255});
        DrawRectangleRoundedLines(exBtn, 0.3f, 8, 1.0f, kPanelLine);
        drawTextF(exerciseMode ? "Exercise: ON" : "Exercise: OFF",
                  lx + 15, y + 13, 30, exerciseMode ? kAccent : kMuted);
        y += 64;
    }

    if (canBuild) {
        Rectangle slowBtn = {static_cast<float>(lx), static_cast<float>(y), 165.0f, 54.0f};
        Rectangle normalBtn = {static_cast<float>(lx + 178), static_cast<float>(y), 120.0f, 54.0f};
        Rectangle fastBtn = {static_cast<float>(lx + 311), static_cast<float>(y), 165.0f, 54.0f};
        Rectangle buttons[] = {slowBtn, normalBtn, fastBtn};
        char speedLabel[32];
        snprintf(speedLabel, sizeof(speedLabel), "%.1fx", timeScale);
        const char* labels[] = {"Slow", speedLabel, "Fast"};
        for (int i = 0; i < 3; ++i) {
            bool hover = CheckCollisionPointRec(GetMousePosition(), buttons[i]);
            bool selected = (i == 0 && timeScale < 1.0f)
                || (i == 1 && fabsf(timeScale - 1.0f) < 0.01f)
                || (i == 2 && timeScale > 1.0f);
            DrawRectangleRounded(buttons[i], 0.3f, 8,
                                 selected ? kAccentSoft
                                          : (hover ? Color{239, 244, 238, 255}
                                                   : Color{250, 251, 248, 255}));
            DrawRectangleRoundedLines(buttons[i], 0.3f, 8, 1.0f,
                                      selected ? kAccent : kPanelLine);
            int tw = measureTextF(labels[i], 24);
            drawTextF(labels[i],
                      static_cast<int>(buttons[i].x + buttons[i].width / 2 - tw / 2),
                      static_cast<int>(buttons[i].y + 14),
                      24,
                      selected ? kAccent : kInk);
        }
        y += 64;
    }

    if (canBuild) {
        Rectangle swBtn = {static_cast<float>(lx), static_cast<float>(y),
                           static_cast<float>(w - 39), 72.0f};
        bool swHover = CheckCollisionPointRec(GetMousePosition(), swBtn);
        DrawRectangleRounded(swBtn, 0.4f, 8,
                             swHover ? Color{52, 132, 96, 255} : kAccent);
        drawTextF("Start Wave", lx + 60, y + 15, 40, WHITE);
        y += 84;
    }

    if (canBuild) {
        Rectangle returnBtn = {static_cast<float>(lx), static_cast<float>(y),
                               static_cast<float>(w - 39), 54.0f};
        bool returnHover = CheckCollisionPointRec(GetMousePosition(), returnBtn);
        DrawRectangleRounded(returnBtn, 0.3f, 8,
                             returnHover ? Color{244, 238, 235, 255} : Color{250, 251, 248, 255});
        DrawRectangleRoundedLines(returnBtn, 0.3f, 8, 1.0f,
                                  returnHover ? Color{190, 95, 70, 255} : kPanelLine);
        const char* returnLabel = "Return to Menu";
        int rw = measureTextF(returnLabel, 28);
        drawTextF(returnLabel,
                  static_cast<int>(returnBtn.x + returnBtn.width / 2 - rw / 2),
                  static_cast<int>(returnBtn.y + 13),
                  28,
                  returnHover ? Color{170, 72, 55, 255} : kInk);
        y += 66;

        Rectangle levelBtn = {static_cast<float>(lx), static_cast<float>(y),
                              static_cast<float>(w - 39), 54.0f};
        bool levelHover = CheckCollisionPointRec(GetMousePosition(), levelBtn);
        DrawRectangleRounded(levelBtn, 0.3f, 8,
                             levelHover ? Color{235, 242, 246, 255} : Color{250, 251, 248, 255});
        DrawRectangleRoundedLines(levelBtn, 0.3f, 8, 1.0f,
                                  levelHover ? Color{70, 120, 170, 255} : kPanelLine);
        const char* levelLabel = "Level Select";
        int lw = measureTextF(levelLabel, 28);
        drawTextF(levelLabel,
                  static_cast<int>(levelBtn.x + levelBtn.width / 2 - lw / 2),
                  static_cast<int>(levelBtn.y + 13),
                  28,
                  levelHover ? Color{50, 100, 155, 255} : kInk);
        y += 66;
    }

    y += 15;
    DrawLine(x + 13, y, x + w - 13, y, kPanelLine);
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
        drawTextF("Exercise: GPA +0.1 / 5s", lx, y, 24, GREEN);
        y += 30;
        drawTextF("Tower fire rate at 65%", lx, y, 24, kWarn);
    }

    EndScissorMode();

    constexpr float kPanelContentHeight = 1120.0f;
    if (kPanelContentHeight > SCREEN_HEIGHT) {
        const float trackX = static_cast<float>(x + w - 10);
        const float trackY = 8.0f;
        const float trackH = static_cast<float>(SCREEN_HEIGHT - 16);
        const float thumbH = std::max(80.0f, trackH * SCREEN_HEIGHT / kPanelContentHeight);
        const float maxScroll = kPanelContentHeight - SCREEN_HEIGHT;
        const float clampedScroll = std::max(0.0f, std::min(panelScrollOffset, maxScroll));
        const float thumbY = trackY + (trackH - thumbH) * (clampedScroll / maxScroll);

        DrawRectangle(static_cast<int>(trackX), static_cast<int>(trackY),
                      4, static_cast<int>(trackH), Color{80, 80, 105, 180});
        DrawRectangle(static_cast<int>(trackX - 2), static_cast<int>(thumbY),
                      8, static_cast<int>(thumbH), Color{180, 180, 210, 220});
    }
}

// ---- Main menu ----

void drawMainMenu(const TextureManager* tm, bool hasSavedGame) {
    ClearBackground(kPaper);

    const char* title = "GPA Defender";
    int tw = measureTextF(title, 78);
    drawTextF(title, SCREEN_WIDTH / 2 - tw / 2, 195, 78, kInk);

    const char* subtitle = "Tower Defense for Academic Survival";
    tw = measureTextF(subtitle, 34);
    drawTextF(subtitle, SCREEN_WIDTH / 2 - tw / 2, 299, 34, kMuted);

    if (tm) {
        const char* menuChars[] = {"ai_coffee", "ai_ai",
                                    "ai_library", "ai_bilibili"};
        constexpr float iconScale = 0.102f;
        constexpr float iconGap = 166.0f;
        constexpr float iconY = 492.0f;
        const float firstX = SCREEN_WIDTH / 2.0f - iconGap * 1.5f;
        for (int i = 0; i < 4; ++i) {
            drawSprite(*tm, menuChars[i], firstX + iconGap * i, iconY, iconScale, WHITE);
        }
    }

    float alpha = 0.5f + 0.5f * sinf(static_cast<float>(GetTime()) * 3.0f);

    auto drawMenuButton = [&](Rectangle rect, const char* label) {
        bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);
        DrawRectangleRounded(rect, 0.22f, 8,
                             hovered ? Color{231, 242, 235, 255} : Color{255, 255, 252, 255});
        DrawRectangleRoundedLines(rect, 0.22f, 8, 1.5f,
                                  hovered ? kAccent : Color{42, 116, 86, static_cast<unsigned char>(110 + alpha * 110)});
        int labelW = measureTextF(label, 34);
        drawTextF(label,
                  static_cast<int>(rect.x + rect.width / 2 - labelW / 2),
                  static_cast<int>(rect.y + 20),
                  34,
                  hovered ? kAccent : Color{42, 116, 86, 255});
    };

    if (hasSavedGame) {
        drawMenuButton(mainMenuContinueRect(), "Continue");
        drawMenuButton(mainMenuNewGameRect(true), "New Game");
        drawMenuButton(mainMenuSaveSlotsRect(), "Save Slots");
    } else {
        drawMenuButton(mainMenuNewGameRect(false), "Start Game");
    }

    const char* info = "Defend your GPA! Place towers, survive waves,";
    tw = measureTextF(info, 46);
    drawTextF(info, SCREEN_WIDTH / 2 - tw / 2, hasSavedGame ? 835 : 767, 46, kMuted);

    info = "and keep all four indicators above threshold.";
    tw = measureTextF(info, 46);
    drawTextF(info, SCREEN_WIDTH / 2 - tw / 2, hasSavedGame ? 897 : 829, 46, kMuted);
}

// ---- Level select ----

void drawLevelSelect(int unlockedLevel, int hoveredLevel, const TextureManager* tm) {
    ClearBackground(kPaper);

    const char* title = "Select Level";
    int tw = measureTextF(title, 64);
    drawTextF(title, SCREEN_WIDTH / 2 - tw / 2, 94, 64, kInk);

    const char* subtitle = "Clear each level to unlock the next year.";
    tw = measureTextF(subtitle, 28);
    drawTextF(subtitle, SCREEN_WIDTH / 2 - tw / 2, 175, 28, kMuted);

    const int cardW = 309;
    const int cardH = 500;
    const int gap = 70;
    const int totalW = cardW * 4 + gap * 3;
    const int startX = SCREEN_WIDTH / 2 - totalW / 2;
    const int cardY = 280;
    const char* levelNames[] = {"Freshman", "Sophomore", "Junior", "Senior"};
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

        Color bg = unlocked ? Color{255, 255, 252, 255} : Color{235, 239, 235, 255};
        Color border = hovered ? kAccent
                               : (unlocked ? kPanelLine : Color{198, 204, 198, 255});
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
                  unlocked ? kInk : kMuted);

        const char* status = unlocked ? "UNLOCKED" : "LOCKED";
        tw = measureTextF(status, unlocked ? 28 : 30);
        drawTextF(status, x + w / 2 - tw / 2, y + 435, unlocked ? 28 : 30,
                  unlocked ? kAccent : Color{180, 85, 85, 255});
    }

    auto drawLevelButton = [&](Rectangle rect, const char* label) {
        bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);
        Color bg = hovered ? kAccentSoft : Color{255, 255, 252, 255};
        Color border = hovered ? kAccent : kPanelLine;
        DrawRectangleRounded(rect, 0.2f, 8, bg);
        DrawRectangleRoundedLines(rect, 0.2f, 8, 1.0f, border);

        tw = measureTextF(label, 30);
        drawTextF(label,
                  static_cast<int>(rect.x + rect.width / 2 - tw / 2),
                  static_cast<int>(rect.y + 20),
                  30,
                  hovered ? kAccent : kInk);
    };

    drawLevelButton(levelSelectRetakeRect(), "Retake ASTI");
    drawLevelButton(levelSelectReturnRect(), "Return to Menu");
}

// ---- Game over / Victory ----

void drawVictoryBadge(Vector2 center, bool finalClear) {
    Color tint = finalClear ? Color{211, 142, 0, 255} : Color{20, 155, 79, 255};
    Color bg = finalClear ? Color{255, 241, 165, 255} : Color{215, 247, 226, 255};
    DrawCircle(static_cast<int>(center.x), static_cast<int>(center.y), 68, bg);
    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 68,
                    finalClear ? Color{245, 213, 84, 255} : Color{177, 232, 194, 255});

    if (finalClear) {
        Vector2 cap[] = {
            {center.x - 48.0f, center.y - 8.0f},
            {center.x, center.y - 30.0f},
            {center.x + 48.0f, center.y - 8.0f},
            {center.x, center.y + 14.0f}
        };
        DrawLineEx(cap[0], cap[1], 7.0f, tint);
        DrawLineEx(cap[1], cap[2], 7.0f, tint);
        DrawLineEx(cap[2], cap[3], 7.0f, tint);
        DrawLineEx(cap[3], cap[0], 7.0f, tint);
        DrawRectangleRounded({center.x - 30.0f, center.y + 2.0f, 60.0f, 24.0f},
                             0.5f, 8, Color{255, 252, 224, 255});
        DrawRectangleRoundedLines({center.x - 30.0f, center.y + 2.0f, 60.0f, 24.0f},
                                  0.5f, 8, 5.0f, tint);
        DrawLineEx({center.x + 42.0f, center.y - 5.0f},
                   {center.x + 42.0f, center.y + 26.0f}, 5.0f, tint);
        DrawCircle(static_cast<int>(center.x + 42.0f), static_cast<int>(center.y + 31.0f), 4, tint);
    } else {
        DrawRectangleRounded({center.x - 30.0f, center.y - 22.0f, 60.0f, 68.0f},
                             0.22f, 8, Color{239, 253, 243, 255});
        DrawRectangleRoundedLines({center.x - 30.0f, center.y - 22.0f, 60.0f, 68.0f},
                                  0.22f, 8, 6.0f, tint);
        DrawCircleLines(static_cast<int>(center.x - 42.0f), static_cast<int>(center.y - 4.0f), 16, tint);
        DrawCircleLines(static_cast<int>(center.x + 42.0f), static_cast<int>(center.y - 4.0f), 16, tint);
        DrawLineEx({center.x, center.y + 46.0f}, {center.x, center.y + 66.0f}, 6.0f, tint);
        DrawLineEx({center.x - 24.0f, center.y + 66.0f}, {center.x + 24.0f, center.y + 66.0f}, 6.0f, tint);
    }
}

void drawStatusRow(Rectangle rect, const char* label, int value, int threshold, Color accent) {
    DrawRectangleRounded(rect, 0.12f, 8, Color{255, 255, 252, 255});
    DrawRectangleRoundedLines(rect, 0.12f, 8, 1.0f, Color{230, 235, 231, 255});

    drawTextF(label, static_cast<int>(rect.x + 24), static_cast<int>(rect.y + 18), 22, kInk);
    std::string valueText = std::to_string(value) + " / 100";
    int valueW = measureTextF(valueText.c_str(), 22);
    drawTextF(valueText.c_str(), static_cast<int>(rect.x + rect.width - valueW - 22),
              static_cast<int>(rect.y + 18), 22, kInk);

    Rectangle track = {rect.x + 24.0f, rect.y + 56.0f, rect.width - 48.0f, 13.0f};
    DrawRectangleRounded(track, 0.45f, 8, Color{223, 229, 237, 255});
    float ratio = std::max(0.0f, std::min(1.0f, value / 100.0f));
    DrawRectangleRounded({track.x, track.y, track.width * ratio, track.height},
                         0.45f, 8, accent);

    float thresholdRatio = std::max(0.0f, std::min(1.0f, threshold / 100.0f));
    float tx = track.x + track.width * thresholdRatio;
    DrawLineEx({tx, track.y - 3.0f}, {tx, track.y + track.height + 3.0f},
               2.0f, Color{255, 125, 125, 235});
    std::string thresholdText = "Threshold: " + std::to_string(threshold);
    int thresholdW = measureTextF(thresholdText.c_str(), 16);
    drawTextF(thresholdText.c_str(), static_cast<int>(rect.x + rect.width - thresholdW - 22),
              static_cast<int>(rect.y + 75), 16, Color{130, 145, 170, 255});
}

float recordGpa(const StageScoreRecord& record) {
    if (record.baseMaxHp <= 0) return 0.0f;
    return std::max(0.0f, std::min(4.0f, 4.0f * record.baseHp / static_cast<float>(record.baseMaxHp)));
}

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
    const char* levels = "Level Select";
    const char* menu = "Return to Menu";

    const char* labels[] = {retry, levels, menu};
    for (int i = 0; i < 3; ++i) {
        Rectangle rect = gameOverOptionRect(i);
        bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
        bool active = selection == i || hover;
        DrawRectangleRounded(rect, 0.22f, 8,
                             active ? Color{255, 244, 207, 245} : Color{250, 251, 248, 230});
        DrawRectangleRoundedLines(rect, 0.22f, 8, 1.0f,
                                  active ? Color{255, 220, 100, 255} : Color{190, 190, 190, 220});
        tw = measureTextF(labels[i], 26);
        drawTextF(labels[i],
                  static_cast<int>(rect.x + rect.width / 2 - tw / 2),
                  static_cast<int>(rect.y + 10),
                  26,
                  active ? kInk : kMuted);
    }
}

void drawTextureFit(const Texture2D& texture, Rectangle dest, Color tint) {
    if (texture.id == 0) return;
    DrawTexturePro(texture,
                   {0.0f, 0.0f,
                    static_cast<float>(texture.width),
                    static_cast<float>(texture.height)},
                   dest, {0.0f, 0.0f}, 0.0f, tint);
}

void drawGambleChoice(const TextureManager* tm) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{0, 0, 0, 155});

    Rectangle prompt{
        SCREEN_WIDTH / 2.0f - 538.0f,
        SCREEN_HEIGHT / 2.0f - 306.0f,
        1076.0f,
        612.0f
    };
    if (tm && tm->gamblePrompt.id != 0) {
        drawTextureFit(tm->gamblePrompt, prompt, WHITE);
    } else {
        DrawRectangleRounded(prompt, 0.04f, 8, Color{255, 246, 230, 255});
        DrawRectangleRoundedLines(prompt, 0.04f, 8, 2.0f, kInk);
        const char* title = "SKIP CLASS?";
        int tw = measureTextF(title, 74);
        drawTextF(title, static_cast<int>(SCREEN_WIDTH / 2 - tw / 2),
                  static_cast<int>(prompt.y + 120), 74, kInk);
    }

    auto drawChoice = [&](Rectangle rect, const Texture2D& texture, const char* label) {
        const bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
        Rectangle drawRect = rect;
        if (hover) {
            drawRect.x -= 9.0f;
            drawRect.y -= 7.0f;
            drawRect.width += 18.0f;
            drawRect.height += 14.0f;
        }
        if (texture.id != 0) {
            drawTextureFit(texture, drawRect, hover ? WHITE : Color{245, 245, 245, 245});
        } else {
            DrawRectangleRounded(drawRect, 0.12f, 8,
                                 hover ? kAccentSoft : Color{255, 255, 252, 255});
            DrawRectangleRoundedLines(drawRect, 0.12f, 8, 2.0f,
                                      hover ? kAccent : kPanelLine);
            int tw = measureTextF(label, 38);
            drawTextF(label,
                      static_cast<int>(drawRect.x + drawRect.width / 2 - tw / 2),
                      static_cast<int>(drawRect.y + drawRect.height / 2 - 23),
                      38, hover ? kAccent : kInk);
        }
    };

    drawChoice(gambleSkipRect(), tm ? tm->gambleSkip : Texture2D{}, "Skip Class");
    drawChoice(gambleNoSkipRect(), tm ? tm->gambleNoSkip : Texture2D{}, "Don't Skip");
}

void drawGambleResult(bool won, const TextureManager* tm) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{0, 0, 0, 165});

    const Texture2D* texture = nullptr;
    if (tm) texture = won ? &tm->gambleWin : &tm->gambleLose;
    Rectangle result{
        SCREEN_WIDTH / 2.0f - 465.0f,
        SCREEN_HEIGHT / 2.0f - 226.0f,
        930.0f,
        452.0f
    };
    if (texture && texture->id != 0) {
        drawTextureFit(*texture, result, WHITE);
    } else {
        DrawRectangleRounded(result, 0.04f, 8, Color{255, 246, 230, 255});
        DrawRectangleRoundedLines(result, 0.04f, 8, 2.0f, kInk);
        const char* title = won ? "Professor didn't roll call." : "Professor rolled call.";
        const char* coins = won ? "Coins +50" : "Coins -50";
        int tw = measureTextF(title, 44);
        drawTextF(title, static_cast<int>(SCREEN_WIDTH / 2 - tw / 2),
                  static_cast<int>(result.y + 125), 44, kInk);
        tw = measureTextF(coins, 52);
        drawTextF(coins, static_cast<int>(SCREEN_WIDTH / 2 - tw / 2),
                  static_cast<int>(result.y + 245), 52,
                  won ? kAccent : Color{180, 85, 85, 255});
    }

    Rectangle btn = gambleResultContinueRect();
    const bool hover = CheckCollisionPointRec(GetMousePosition(), btn);
    DrawRectangleRounded(btn, 0.22f, 8,
                         hover ? kAccentSoft : Color{255, 255, 252, 245});
    DrawRectangleRoundedLines(btn, 0.22f, 8, 1.5f,
                              hover ? kAccent : kPanelLine);
    const char* label = "Continue";
    int tw = measureTextF(label, 30);
    drawTextF(label,
              static_cast<int>(btn.x + btn.width / 2 - tw / 2),
              static_cast<int>(btn.y + 17),
              30, hover ? kAccent : kInk);
}

void drawVictory(int selection, bool hasNextLevel,
                 const StageScoreRecord* currentScore,
                 const std::vector<StageScoreRecord>& stageScores) {
    if (hasNextLevel) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                      Color{0, 0, 0, 180});

        const char* msg = "VICTORY!";
        int tw = measureTextF(msg, 68);
        drawTextF(msg, SCREEN_WIDTH / 2 - tw / 2, 338, 68, Color{80, 220, 80, 255});

        const char* hint = "You have successfully defended your GPA!";
        tw = measureTextF(hint, 23);
        drawTextF(hint, SCREEN_WIDTH / 2 - tw / 2, 442, 23, LIGHTGRAY);

        const char* labels[] = {"Continue to Next Level", "Level Select", "Return to Menu"};
        for (int i = 0; i < 3; ++i) {
            Rectangle rect = victoryOptionRect(i, hasNextLevel);
            bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
            bool active = selection == i || hover;
            DrawRectangleRounded(rect, 0.22f, 8,
                                 active ? Color{220, 246, 226, 245} : Color{250, 251, 248, 230});
            DrawRectangleRoundedLines(rect, 0.22f, 8, 1.0f,
                                      active ? Color{80, 220, 80, 255} : Color{190, 190, 190, 220});
            tw = measureTextF(labels[i], 26);
            drawTextF(labels[i],
                      static_cast<int>(rect.x + rect.width / 2 - tw / 2),
                      static_cast<int>(rect.y + 10),
                      26,
                      active ? kInk : kMuted);
        }
        return;
    }

    const bool finalClear = !hasNextLevel;
    const Color primary = finalClear ? Color{210, 137, 0, 255} : Color{23, 140, 69, 255};
    const Color primarySoft = finalClear ? Color{255, 247, 211, 255} : Color{232, 249, 238, 255};
    const Color primaryLine = finalClear ? Color{236, 204, 91, 255} : Color{164, 226, 183, 255};
    const char* stageNames[] = {"Freshman", "Sophomore", "Junior", "Senior"};

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, primarySoft);

    Rectangle panel = {SCREEN_WIDTH / 2.0f - 670.0f, 34.0f, 1340.0f, 1180.0f};
    DrawRectangleRounded(panel, 0.035f, 10, Color{255, 255, 252, 255});
    DrawRectangleRoundedLines(panel, 0.035f, 10, 1.5f, primaryLine);

    drawVictoryBadge({SCREEN_WIDTH / 2.0f, 126.0f}, finalClear);

    const char* title = finalClear ? "Happy Graduation!" : "Semester Survived!";
    int tw = measureTextF(title, 66);
    drawTextF(title, SCREEN_WIDTH / 2 - tw / 2, 222, 66, primary);

    const char* subtitle = finalClear
        ? "Outstanding! Review your academic record and final status."
        : "Congratulations! You balanced your life and maintained your GPA this period.";
    tw = measureTextF(subtitle, 28);
    drawTextF(subtitle, SCREEN_WIDTH / 2 - tw / 2, 305, 28, Color{36, 56, 86, 255});

    Rectangle recordCard = {panel.x + 58.0f, panel.y + 374.0f, 570.0f, 610.0f};
    Rectangle statusCard = {panel.x + 712.0f, panel.y + 374.0f, 570.0f, 610.0f};
    DrawRectangleRounded(recordCard, 0.035f, 8, Color{247, 249, 252, 255});
    DrawRectangleRoundedLines(recordCard, 0.035f, 8, 1.0f, Color{215, 224, 235, 255});
    DrawRectangleRounded(statusCard, 0.035f, 8, Color{247, 249, 252, 255});
    DrawRectangleRoundedLines(statusCard, 0.035f, 8, 1.0f, Color{215, 224, 235, 255});

    drawTextF("Academic Record", static_cast<int>(recordCard.x + 52),
              static_cast<int>(recordCard.y + 42), 32, Color{32, 53, 82, 255});
    drawTextF("Final Status", static_cast<int>(statusCard.x + 52),
              static_cast<int>(statusCard.y + 42), 32, Color{32, 53, 82, 255});
    DrawLine(static_cast<int>(recordCard.x + 42), static_cast<int>(recordCard.y + 92),
             static_cast<int>(recordCard.x + recordCard.width - 42), static_cast<int>(recordCard.y + 92),
             Color{218, 226, 235, 255});
    DrawLine(static_cast<int>(statusCard.x + 42), static_cast<int>(statusCard.y + 92),
             static_cast<int>(statusCard.x + statusCard.width - 42), static_cast<int>(statusCard.y + 92),
             Color{218, 226, 235, 255});

    float gpaValue = currentScore ? recordGpa(*currentScore) : 0.0f;
    char gpaText[32];
    std::snprintf(gpaText, sizeof(gpaText), "%.2f", gpaValue);
    drawTextF("Final GPA", static_cast<int>(recordCard.x + 42),
              static_cast<int>(recordCard.y + 150), 28, Color{82, 105, 134, 255});
    int gpaW = measureTextF(gpaText, 48);
    drawTextF(gpaText, static_cast<int>(recordCard.x + recordCard.width - gpaW - 54),
              static_cast<int>(recordCard.y + 126), 48, Color{84, 75, 230, 255});

    auto drawRecordRow = [&](float y, const char* label, const std::string& value) {
        Rectangle row = {recordCard.x + 42.0f, y, recordCard.width - 84.0f, 70.0f};
        DrawRectangleRounded(row, 0.12f, 8, Color{255, 255, 252, 255});
        DrawRectangleRoundedLines(row, 0.12f, 8, 1.0f, Color{232, 236, 241, 255});
        drawTextF(label, static_cast<int>(row.x + 22), static_cast<int>(row.y + 21),
                  22, Color{82, 105, 134, 255});
        int valueW = measureTextF(value.c_str(), 22);
        drawTextF(value.c_str(), static_cast<int>(row.x + row.width - valueW - 22),
                  static_cast<int>(row.y + 21), 22, kInk);
    };

    int waves = currentScore ? currentScore->waveIndex + 1 : 0;
    drawRecordRow(recordCard.y + 220.0f, "Waves Survived", std::to_string(std::max(0, waves)));
    std::string milestone = "Stage Cleared";
    if (currentScore && currentScore->level >= 1 && currentScore->level <= 4) {
        milestone = std::string(stageNames[currentScore->level - 1]) + " Year Cleared";
    }
    drawRecordRow(recordCard.y + 306.0f, "Milestone", milestone);

    DrawLine(static_cast<int>(recordCard.x + 42), static_cast<int>(recordCard.y + 430),
             static_cast<int>(recordCard.x + recordCard.width - 42), static_cast<int>(recordCard.y + 430),
             Color{218, 226, 235, 255});
    const char* journey = "4-YEAR GPA JOURNEY";
    tw = measureTextF(journey, 20);
    drawTextF(journey, static_cast<int>(recordCard.x + recordCard.width / 2 - tw / 2),
              static_cast<int>(recordCard.y + 462), 20, Color{139, 153, 178, 255});

    for (int i = 0; i < 4; ++i) {
        const StageScoreRecord* record = nullptr;
        for (const StageScoreRecord& candidate : stageScores) {
            if (candidate.level == i + 1) {
                record = &candidate;
                break;
            }
        }
        float cx = recordCard.x + 90.0f + i * 130.0f;
        bool active = currentScore && currentScore->level == i + 1;
        DrawCircle(static_cast<int>(cx), static_cast<int>(recordCard.y + 524), 30,
                   active ? primarySoft : Color{255, 255, 252, 255});
        DrawCircleLines(static_cast<int>(cx), static_cast<int>(recordCard.y + 524), 30,
                        active ? primary : Color{182, 194, 255, 255});
        std::string score = "--";
        if (record != nullptr) {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%.1f", recordGpa(*record));
            score = buf;
        }
        int scoreW = measureTextF(score.c_str(), 24);
        drawTextF(score.c_str(), static_cast<int>(cx - scoreW / 2),
                  static_cast<int>(recordCard.y + 510), 24, active ? primary : Color{85, 80, 222, 255});
        std::string stageLabel = stageNames[i];
        std::transform(stageLabel.begin(), stageLabel.end(), stageLabel.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
        int labelW = measureTextF(stageLabel.c_str(), 13);
        drawTextF(stageLabel.c_str(), static_cast<int>(cx - labelW / 2),
                  static_cast<int>(recordCard.y + 565), 13, Color{139, 153, 178, 255});
    }

    if (currentScore) {
        drawStatusRow({statusCard.x + 42.0f, statusCard.y + 120.0f, statusCard.width - 84.0f, 86.0f},
                      "Academic Knowledge", currentScore->academic,
                      currentScore->thresholdAcademic, indicatorColor(0));
        drawStatusRow({statusCard.x + 42.0f, statusCard.y + 232.0f, statusCard.width - 84.0f, 86.0f},
                      "Physical Energy", currentScore->physical,
                      currentScore->thresholdPhysical, Color{32, 195, 91, 255});
        drawStatusRow({statusCard.x + 42.0f, statusCard.y + 344.0f, statusCard.width - 84.0f, 86.0f},
                      "Mental Health", currentScore->mental,
                      currentScore->thresholdMental, Color{166, 80, 232, 255});
        drawStatusRow({statusCard.x + 42.0f, statusCard.y + 456.0f, statusCard.width - 84.0f, 86.0f},
                      "Social Connections", currentScore->connection,
                      currentScore->thresholdConnection, Color{232, 70, 145, 255});
    }

    DrawLine(static_cast<int>(panel.x + 58), 1048,
             static_cast<int>(panel.x + panel.width - 58), 1048, Color{215, 224, 235, 255});

    const char* nextLevel = "Continue Next Term";
    const char* levels = "Level Select";
    const char* menu = "Main Menu";

    if (hasNextLevel) {
        const char* labels[] = {nextLevel, levels, menu};
        for (int i = 0; i < 3; ++i) {
            Rectangle rect = victoryOptionRect(i, hasNextLevel);
            bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
            bool active = selection == i || hover;
            bool primaryButton = i == 0;
            DrawRectangleRounded(rect, 0.22f, 8,
                                 primaryButton ? (active ? Color{20, 160, 82, 255} : Color{20, 170, 85, 255})
                                               : (active ? Color{246, 250, 247, 255} : Color{255, 255, 252, 255}));
            DrawRectangleRoundedLines(rect, 0.22f, 8, 1.0f,
                                      primaryButton ? Color{20, 145, 75, 255}
                                                    : (active ? primary : Color{196, 207, 222, 255}));
            tw = measureTextF(labels[i], 24);
            drawTextF(labels[i],
                      static_cast<int>(rect.x + rect.width / 2 - tw / 2),
                      static_cast<int>(rect.y + 19),
                      24,
                      primaryButton ? WHITE : kInk);
        }
    } else {
        const char* labels[] = {levels, menu};
        for (int i = 0; i < 2; ++i) {
            Rectangle rect = victoryOptionRect(i, hasNextLevel);
            bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
            bool active = selection == i || hover;
            bool primaryButton = i == 0;
            DrawRectangleRounded(rect, 0.22f, 8,
                                 primaryButton ? (active ? Color{212, 142, 0, 255} : Color{225, 153, 0, 255})
                                               : (active ? Color{255, 251, 238, 255} : Color{255, 255, 252, 255}));
            DrawRectangleRoundedLines(rect, 0.22f, 8, 1.0f,
                                      primaryButton ? Color{196, 126, 0, 255}
                                                    : (active ? primary : Color{196, 207, 222, 255}));
            tw = measureTextF(labels[i], 24);
            drawTextF(labels[i],
                      static_cast<int>(rect.x + rect.width / 2 - tw / 2),
                      static_cast<int>(rect.y + 19),
                      24,
                      primaryButton ? WHITE : kInk);
        }
    }

    const char* tip = finalClear
        ? "Developer Note: Thank you for playing GPA Defender!"
        : "Tip: The next term will bring tougher deadlines. Spend your gold wisely.";
    tw = measureTextF(tip, 22);
    drawTextF(tip, SCREEN_WIDTH / 2 - tw / 2, 1225, 22, Color{126, 143, 164, 255});
}

// ---- Questionnaire / ASTI ----

void drawQuestionnaire(const Questionnaire& q, int current,
                       const std::vector<int>& answers) {
    ClearBackground(kPaper);

    const auto& questions = q.getQuestions();
    if (current < 0 || current >= static_cast<int>(questions.size())) return;

    const auto& question = questions[current];

    const char* title = "ASTI Personality Quiz";
    int tw = measureTextF(title, 57);
    drawTextF(title, SCREEN_WIDTH / 2 - tw / 2, 54, 54, kInk);

    std::string progress = "Question " + std::to_string(current + 1) + " / " +
                           std::to_string(static_cast<int>(questions.size()));
    tw = measureTextF(progress.c_str(), 50);
    drawTextF(progress.c_str(), SCREEN_WIDTH / 2 - tw / 2, 132, 34, kMuted);

    DrawRectangleRounded(
        {78.0f, 195.0f, static_cast<float>(SCREEN_WIDTH - 156), 170.0f}, 0.2f, 8,
        Color{255, 255, 252, 255});
    DrawRectangleRoundedLines(
        {78.0f, 195.0f, static_cast<float>(SCREEN_WIDTH - 156), 170.0f}, 0.2f, 8, 1.0f,
        kPanelLine);
    drawTextF(question.prompt.c_str(), 104, 236, 46, kInk);

    for (size_t i = 0; i < question.options.size(); ++i) {
        const int optionHeight = 170;
        const int optionStep = 195;
        int oy = 400 + static_cast<int>(i) * optionStep;
        bool selected = (current < static_cast<int>(answers.size()) &&
                         answers[current] == static_cast<int>(i));
        Color bg = selected ? kAccentSoft : Color{255, 255, 252, 255};
        Rectangle optionRect = questionnaireOptionRect(i);
        bool hovered = CheckCollisionPointRec(GetMousePosition(), optionRect);
        if (hovered && !selected) bg = Color{240, 246, 241, 255};
        DrawRectangleRounded(optionRect, 0.2f, 8, bg);
        DrawRectangleRoundedLines(optionRect, 0.2f, 8, 1.0f,
                                  selected ? kAccent : (hovered ? kAccent : kPanelLine));

        std::string label = std::to_string(i + 1) + ".";
        drawTextF(label.c_str(), 156, oy + 24, 38, selected ? kAccent : kMuted);

        drawTextF(question.options[i].text.c_str(), 195, oy + 24, 38,
                  selected ? kInk : kMuted);

        std::string effStr;
        for (const auto& eff : question.options[i].effects) {
            const char* indName[] = {"Academic", "Physical", "Mental", "Connection"};
            if (!effStr.empty()) effStr += ", ";
            char sign = eff.delta >= 0 ? '+' : '-';
            effStr += std::string(indName[static_cast<int>(eff.indicator)]) +
                      " " + sign + std::to_string(abs(eff.delta));
        }
        drawTextF(effStr.c_str(), 195, oy + optionHeight - 48, 32, kMuted);
    }

    const char* hint = "Click an option, or press 1-4.";
    tw = measureTextF(hint, 52);
    drawTextF(hint, SCREEN_WIDTH / 2 - tw / 2, SCREEN_HEIGHT - 80, 38, kMuted);
}

void drawAstiSummary(const AstiResult& result) {
    ClearBackground(kPaper);

    const char* title = "Your ASTI Type";
    int tw = measureTextF(title, 65);
    drawTextF(title, SCREEN_WIDTH / 2 - tw / 2, 94, 60, kInk);

    const char* subtitle = "Attend School Type Indicator";
    tw = measureTextF(subtitle, 36);
    drawTextF(subtitle, SCREEN_WIDTH / 2 - tw / 2, 174, 32, kMuted);

    DrawRectangleRounded(
        {156.0f, 260.0f, static_cast<float>(SCREEN_WIDTH - 312), 195.0f}, 0.2f, 8,
        Color{255, 255, 252, 255});
    DrawRectangleRoundedLines(
        {156.0f, 260.0f, static_cast<float>(SCREEN_WIDTH - 312), 195.0f}, 0.2f, 8, 1.0f,
        kPanelLine);

    drawTextF("Result Type", 202, 296, 34, kMuted);

    std::string tagText;
    for (size_t i = 0; i < result.tags.size(); ++i) {
        if (!tagText.empty()) tagText += " / ";
        tagText += result.tags[i];
    }
    if (tagText.empty()) tagText = "Unknown";

    tw = measureTextF(tagText.c_str(), 57);
    drawTextF(tagText.c_str(), SCREEN_WIDTH / 2 - tw / 2, 354, 52, kAccent);

    const int cardX = 195;
    const int cardY = 533;
    const int cardW = SCREEN_WIDTH - 390;
    const int rowH = 81;

    drawTextF("Final Thresholds", cardX, 486, 34, kInk);
    drawTextF("Falling below any threshold means failure.", cardX + 260, 491, 27, kMuted);

    const char* names[] = {"Academic", "Physical", "Mental", "Connection"};
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
            0.15f, 8, (i % 2 == 0) ? Color{255, 255, 252, 255} : Color{244, 247, 242, 255});
        DrawRectangleRoundedLines(
            {static_cast<float>(cardX), static_cast<float>(y),
             static_cast<float>(cardW), static_cast<float>(rowH - 10)},
            0.15f, 8, 1.0f, kPanelLine);
        DrawCircle(cardX + 36, y + 35, 12, c);
        drawTextF(names[i], cardX + 68, y + 21, 31, kInk);

        std::string valueStr = std::to_string(values[i]);
        int valueW = measureTextF(valueStr.c_str(), 36);
        drawTextF(valueStr.c_str(), cardX + cardW - valueW - 44, y + 18, 36, c);
    }

    Rectangle continueRect = astiContinueRect();
    bool hovered = CheckCollisionPointRec(GetMousePosition(), continueRect);
    float alpha = 0.5f + 0.5f * sinf(static_cast<float>(GetTime()) * 3.0f);
    DrawRectangleRounded(continueRect, 0.22f, 8,
                         hovered ? Color{231, 242, 235, 255} : Color{255, 255, 252, 255});
    DrawRectangleRoundedLines(continueRect, 0.22f, 8, 1.2f,
                              hovered ? kAccent : Color{42, 116, 86, static_cast<unsigned char>(110 + alpha * 110)});
    const char* hint = "Continue";
    tw = measureTextF(hint, 30);
    drawTextF(hint,
              static_cast<int>(continueRect.x + continueRect.width / 2 - tw / 2),
              static_cast<int>(continueRect.y + 19),
              30,
              hovered ? kAccent : Color{42, 116, 86, 255});
}

}  // namespace frontend
