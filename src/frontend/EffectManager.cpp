#include "frontend/EffectManager.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "frontend/Renderer.h"

namespace frontend {
namespace {

Vector2 toScreen(const Vector2& world) {
    return {world.x + MAP_OFFSET_X, world.y + MAP_OFFSET_Y};
}

float clamp01(float value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

Color fade(Color color, float alpha) {
    color.a = static_cast<unsigned char>(static_cast<float>(color.a) * clamp01(alpha));
    return color;
}

Vector2 lerp(Vector2 a, Vector2 b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

float length(Vector2 v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

Vector2 normalize(Vector2 v) {
    float len = length(v);
    if (len < 0.001f) return {1.0f, 0.0f};
    return {v.x / len, v.y / len};
}

Vector2 perpendicular(Vector2 v) {
    return {-v.y, v.x};
}

void drawJaggedLine(Vector2 a, Vector2 b, float seed, Color color, float width) {
    constexpr int segments = 5;
    Vector2 prev = a;
    Vector2 dir = normalize({b.x - a.x, b.y - a.y});
    Vector2 normal = perpendicular(dir);
    for (int i = 1; i <= segments; ++i) {
        float t = static_cast<float>(i) / segments;
        Vector2 p = lerp(a, b, t);
        if (i < segments) {
            float wobble = std::sin(seed * 5.13f + i * 1.91f) * 8.0f;
            p.x += normal.x * wobble;
            p.y += normal.y * wobble;
        }
        DrawLineEx(prev, p, width, color);
        prev = p;
    }
}

} // namespace

void EffectManager::spawn(const TowerAttackEvent& event) {
    Effect effect;
    effect.kind = event.kind;
    effect.origin = {event.origin.x, event.origin.y};
    effect.direction = normalize({event.direction.x, event.direction.y});
    effect.range = event.range;
    effect.seed = static_cast<float>(GetRandomValue(0, 10000)) / 1000.0f;
    effect.targets.reserve(event.targets.size());
    for (const Vector2D& target : event.targets) {
        effect.targets.push_back({target.x, target.y});
    }

    switch (event.kind) {
    case TowerEffectKind::Coffee:
        effect.duration = 0.28f;
        break;
    case TowerEffectKind::AI:
        effect.duration = 0.24f;
        break;
    case TowerEffectKind::Library:
        effect.duration = 0.72f;
        break;
    case TowerEffectKind::Class:
        effect.duration = 0.30f;
        break;
    case TowerEffectKind::Bilibili:
        effect.duration = 0.20f;
        break;
    }

    effects.push_back(std::move(effect));
    if (effects.size() > 160) {
        effects.erase(effects.begin(), effects.begin() + static_cast<long long>(effects.size() - 160));
    }
}

void EffectManager::spawnAll(const std::vector<TowerAttackEvent>& events) {
    for (const TowerAttackEvent& event : events) {
        spawn(event);
    }
}

void EffectManager::update(float dt) {
    if (dt < 0.0f) return;
    for (Effect& effect : effects) {
        effect.age += dt;
    }
    effects.erase(
        std::remove_if(effects.begin(), effects.end(),
            [](const Effect& effect) { return effect.age >= effect.duration; }),
        effects.end());
}

void EffectManager::draw() const {
    BeginBlendMode(BLEND_ADDITIVE);
    for (const Effect& effect : effects) {
        float t = clamp01(effect.age / effect.duration);
        float alpha = 1.0f - t;
        Vector2 origin = toScreen(effect.origin);

        switch (effect.kind) {
        case TowerEffectKind::Coffee:
            for (Vector2 targetWorld : effect.targets) {
                Vector2 target = toScreen(targetWorld);
                Vector2 p = lerp(origin, target, clamp01(t * 1.35f));
                DrawLineEx(origin, p, 5.0f, fade(Color{255, 190, 70, 190}, alpha));
                DrawCircleV(p, 7.0f + 9.0f * t, fade(Color{255, 235, 135, 220}, alpha));
                DrawCircleLines(static_cast<int>(target.x), static_cast<int>(target.y),
                                18.0f + 26.0f * t, fade(Color{255, 155, 55, 180}, alpha));
            }
            break;

        case TowerEffectKind::AI:
            for (Vector2 targetWorld : effect.targets) {
                Vector2 target = toScreen(targetWorld);
                drawJaggedLine(origin, target, effect.seed + target.x * 0.01f,
                               fade(Color{70, 225, 255, 220}, alpha), 4.0f);
                drawJaggedLine(origin, target, effect.seed + target.y * 0.02f + 11.0f,
                               fade(Color{190, 90, 255, 150}, alpha * 0.8f), 2.0f);
                DrawCircleV(target, 9.0f + 8.0f * t, fade(Color{120, 240, 255, 180}, alpha));
            }
            DrawCircleLines(static_cast<int>(origin.x), static_cast<int>(origin.y),
                            34.0f + 22.0f * t, fade(Color{115, 225, 255, 130}, alpha));
            break;

        case TowerEffectKind::Library:
            {
                float radius = effect.range * (0.25f + 0.75f * t);
                DrawCircleLines(static_cast<int>(origin.x), static_cast<int>(origin.y),
                                radius, fade(Color{75, 180, 255, 190}, alpha));
                DrawCircleLines(static_cast<int>(origin.x), static_cast<int>(origin.y),
                                radius * 0.72f, fade(Color{120, 220, 255, 120}, alpha * 0.7f));
                for (Vector2 targetWorld : effect.targets) {
                    Vector2 target = toScreen(targetWorld);
                    DrawCircleLines(static_cast<int>(target.x), static_cast<int>(target.y),
                                    18.0f + 16.0f * t, fade(Color{85, 170, 255, 120}, alpha));
                }
            }
            break;

        case TowerEffectKind::Class:
            for (Vector2 targetWorld : effect.targets) {
                Vector2 target = toScreen(targetWorld);
                DrawLineEx(origin, target, 5.0f, fade(Color{255, 228, 92, 160}, alpha));
                DrawLineEx(origin, target, 2.0f, fade(Color{255, 255, 210, 230}, alpha));
                static const char* formulas[] = {"x^2", "d/dx", "lim", "sum", "pi", "sqrt"};
                Vector2 dir = normalize({target.x - origin.x, target.y - origin.y});
                Vector2 normal = perpendicular(dir);
                for (int i = 0; i < 4; ++i) {
                    float ft = clamp01(t + i * 0.16f);
                    Vector2 p = lerp(origin, target, ft);
                    float wobble = std::sin(effect.seed + i * 1.7f) * 14.0f;
                    p.x += normal.x * wobble;
                    p.y += normal.y * wobble;
                    const char* formula = formulas[(static_cast<int>(effect.seed * 10.0f) + i) % 6];
                    int size = i == 0 ? 24 : 20;
                    drawTextF(formula, static_cast<int>(p.x), static_cast<int>(p.y), size,
                              fade(Color{255, 245, 160, 230}, alpha));
                }
                DrawCircleLines(static_cast<int>(target.x), static_cast<int>(target.y),
                                18.0f + 22.0f * t, fade(Color{255, 175, 65, 180}, alpha));
            }
            break;

        case TowerEffectKind::Bilibili:
            {
                Vector2 end = origin;
                if (!effect.targets.empty()) {
                    end = toScreen(effect.targets.front());
                } else {
                    end = {origin.x + effect.direction.x * effect.range,
                           origin.y + effect.direction.y * effect.range};
                }
                Vector2 dir = normalize({end.x - origin.x, end.y - origin.y});
                Vector2 normal = perpendicular(dir);
                float pulse = 1.0f + std::sin((effect.age + effect.seed) * 42.0f) * 0.25f;
                for (int i = -1; i <= 1; ++i) {
                    Vector2 offset{normal.x * i * 9.0f, normal.y * i * 9.0f};
                    DrawLineEx({origin.x + offset.x, origin.y + offset.y},
                               {end.x + offset.x, end.y + offset.y},
                               (i == 0 ? 11.0f : 4.0f) * pulse,
                               fade(i == 0 ? Color{255, 75, 210, 210} : Color{65, 210, 255, 170}, alpha));
                }
                DrawCircleV(end, 13.0f + 10.0f * t, fade(Color{255, 95, 230, 210}, alpha));
            }
            break;
        }
    }
    EndBlendMode();
}

void EffectManager::clear() {
    effects.clear();
}

} // namespace frontend
