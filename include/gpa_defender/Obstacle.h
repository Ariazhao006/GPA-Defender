#pragma once
#ifndef OBSTACLE_H
#define OBSTACLE_H

#include "gpa_defender/Vector2D.h"

/**
 * @class Obstacle
 * @brief Player-placed obstacle (standalone class, NOT a tower).
 *
 * An Obstacle does not attack. It only occupies a rectangular area on
 * the map and forces the path-finding module to recompute enemy paths
 * around it. Enemies must not cross an obstacle.
 *
 * The map module should call getBlockingRect() after placement to update
 * the navigation grid / waypoints.
 */
class Obstacle {
private:
    int cost;            // gold cost to place the obstacle
    Vector2D position;   // center coordinate on the map
    float width;         // rectangle width
    float height;        // rectangle height
    bool blocksPath;     // whether this obstacle blocks pathfinding (default true)
    bool placed;         // whether the obstacle has been placed on the map

public:
    explicit Obstacle(int cost = 30, float width = 40.0f, float height = 40.0f);

    // Place this obstacle at the given map position.
    void place(const Vector2D& pos);

    // Axis-aligned bounding rectangle, used by the map module to
    // recompute enemy paths around this obstacle.
    Rect getBlockingRect() const;

    bool isBlocking() const { return blocksPath; }
    bool isPlaced() const { return placed; }

    int getCost() const { return cost; }
    Vector2D getPosition() const { return position; }
    float getWidth() const { return width; }
    float getHeight() const { return height; }
};

#endif // OBSTACLE_H

