#pragma once
// ========================================
// File: Vector2D.h
// ========================================
#pragma once
#ifndef VECTOR2D_H
#define VECTOR2D_H

#include <cmath>

/**
 * @brief ��ά�����ṹ�壬���ڱ�ʾ������������
 */
struct Vector2D {
    float x;
    float y;

    float distanceTo(const Vector2D& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    Vector2D normalize() const {
        float len = std::sqrt(x * x + y * y);
        if (len == 0.0f) return { 0.0f, 0.0f };
        return { x / len, y / len };
    }
};

struct Rect {
    float x, y;
    float width, height;
};

/**
 * @brief �㷨��AABB (Axis-Aligned Bounding Box) ������ײ������
 * @param a ��һ�����Σ�������˵���ײ�У�
 * @param b �ڶ������Σ������ϰ����ռ�ݺУ�
 */
inline bool checkCollision(const Rect& a, const Rect& b) {
    return (a.x < b.x + b.width && a.x + a.width > b.x &&
        a.y < b.y + b.height && a.y + a.height > b.y);
}

#endif // VECTOR2D_H
