#pragma once

#include <vector>

#include "gpa_defender/DefenseTower.h"
#include "raylib.h"

namespace frontend {

class EffectManager {
public:
    void spawn(const TowerAttackEvent& event);
    void spawnAll(const std::vector<TowerAttackEvent>& events);
    void update(float dt);
    void draw() const;
    void clear();

private:
    struct Effect {
        TowerEffectKind kind = TowerEffectKind::Coffee;
        Vector2 origin{0.0f, 0.0f};
        std::vector<Vector2> targets;
        Vector2 direction{1.0f, 0.0f};
        float range = 0.0f;
        float age = 0.0f;
        float duration = 0.35f;
        float seed = 0.0f;
    };

    std::vector<Effect> effects;
};

} // namespace frontend
