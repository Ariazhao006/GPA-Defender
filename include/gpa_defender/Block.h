#pragma once
#ifndef BLOCK_H
#define BLOCK_H

#include <vector>
#include "gpa_defender/Vector2D.h"

// 地形类型枚举
enum class TileType {
    Wall = 0,       // 0: 静态墙体/深坑（绝对不可走，不可放塔）
    Path = 1,       // 1: 普通地面通道（怪物默认行走，可放 Obstacle 改变路线）
    Highland = 2,   // 2: 高台（仅限放 DefenseTower，不可走）
    Spawn = 3,      // 3: 红门出怪点
    Base = 4        // 4: 蓝门保护目标
};

// 单个区块的运行时结构体
struct BlockCell {
    int row;
    int col;
    TileType type;
    bool isWalkable;    // 当前是否可通行（供寻路模块实时查询）
    bool hasObstacle;   // 当前是否被玩家放置了动态障碍物 (Obstacle)
    bool hasTower;      // 当前是否放置了防御塔
    Rect boundingBox;   // 物理 AABB 碰撞盒 (x, y, width, height)
};

class Block {
private:
    std::vector<std::vector<BlockCell>> grid;
    int totalRows;
    int totalCols;
    float blockSize; // 每个方块的像素边长（默认 64.0f）

public:
    explicit Block(float size = 64.0f);

    // --- 核心初始化 ---
    // 根据外部读取出的纯数字矩阵，实例化物理区块
    void buildBlocks(const std::vector<std::vector<int>>& numericMap);

    // --- 空间坐标换算 ---
    bool worldToGrid(float worldX, float worldY, int& outRow, int& outCol) const;
    Vector2D getBlockCenter(int row, int col) const;
    Rect getBlockRect(int row, int col) const;

    // --- 供寻路模块调用的路况查询接口 ---
    bool isWalkable(int row, int col) const;
    
    // 查询指定 AABB 矩形是否与任何不可通行的 Block 发生物理重叠
    bool checkOverlapWithObstacles(const Rect& entityRect) const;

    // --- 供建造/放置模块调用的状态更新接口 ---
    bool canPlaceObstacle(int row, int col) const;
    bool canPlaceTower(int row, int col) const;

    // 放置或拆除动态障碍物，立刻触发阻挡状态切换
    bool placeObstacleAt(int row, int col);
    bool removeObstacleAt(int row, int col);
    
    // 放置防御塔标记
    bool placeTowerAt(int row, int col);

    // 获取网格只读引用
    const std::vector<std::vector<BlockCell>>& getGrid() const { return grid; }
    int getRows() const { return totalRows; }
    int getCols() const { return totalCols; }
};

#endif // BLOCK_H
