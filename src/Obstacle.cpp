#include "gpa_defender/Obstacle.h"

Obstacle::Obstacle(int cost, float width, float height)
    : cost(cost), position{ 0.0f, 0.0f },
    width(width), height(height),
    blocksPath(true), placed(false) {}

void Obstacle::place(const Vector2D& pos) {
    position = pos;
    placed = true;
}

// The blocking rectangle is centered on `position` so the map module can
// directly feed it into AABB-based path-finding logic.
Rect Obstacle::getBlockingRect() const {
    Rect r;
    r.x = position.x - width / 2.0f;
    r.y = position.y - height / 2.0f;
    r.width = width;
    r.height = height;
    return r;
}

