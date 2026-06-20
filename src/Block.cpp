#include "gpa_defender/Block.h"
#include <iostream>

Block::Block(float size) 
    : totalRows(0), totalCols(0), blockSize(size) {}

void Block::buildBlocks(const std::vector<std::vector<int>>& numericMap) {
    if (numericMap.empty() || numericMap[0].empty()) return;

    totalRows = static_cast<int>(numericMap.size());
    totalCols = static_cast<int>(numericMap[0].size());
    grid.assign(totalRows, std::vector<BlockCell>(totalCols));

    for (int r = 0; r < totalRows; ++r) {
        for (int c = 0; c < totalCols; ++c) {
            BlockCell cell;
            cell.row = r;
            cell.col = c;
            cell.type = static_cast<TileType>(numericMap[r][c]);
            cell.hasObstacle = false;
            cell.hasTower = false;

            // 计算该区块在世界坐标系下的包围盒
            cell.boundingBox = {
                c * blockSize,       // 左上角 X
                r * blockSize,       // 左上角 Y
                blockSize,           // 宽度
                blockSize            // 高度
            };

            // 初始物理规则判定：只有地面通道(1)、红门(3)、蓝门(4)默认可通行
            if (cell.type == TileType::Path || 
                cell.type == TileType::Spawn || 
                cell.type == TileType::Base) {
                cell.isWalkable = true;
            } else {
                cell.isWalkable = false; // 墙体(0)和高台(2)绝对不可走
            }

            grid[r][c] = cell;
        }
    }
    std::cout << "[Block] Built grid with " << totalRows * totalCols << " cells.\n";
}

bool Block::worldToGrid(float worldX, float worldY, int& outRow, int& outCol) const {
    if (blockSize <= 0.0f) return false;
    outCol = static_cast<int>(worldX / blockSize);
    outRow = static_cast<int>(worldY / blockSize);
    return (outRow >= 0 && outRow < totalRows && outCol >= 0 && outCol < totalCols);
}

Vector2D Block::getBlockCenter(int row, int col) const {
    return {
        col * blockSize + blockSize * 0.5f,
        row * blockSize + blockSize * 0.5f
    };
}

Rect Block::getBlockRect(int row, int col) const {
    if (row >= 0 && row < totalRows && col >= 0 && col < totalCols) {
        return grid[row][col].boundingBox;
    }
    return {0, 0, 0, 0};
}

bool Block::isWalkable(int row, int col) const {
    if (row < 0 || row >= totalRows || col < 0 || col >= totalCols) return false;
    return grid[row][col].isWalkable;
}

bool Block::checkOverlapWithObstacles(const Rect& entityRect) const {
    for (int r = 0; r < totalRows; ++r) {
        for (int c = 0; c < totalCols; ++c) {
            if (!grid[r][c].isWalkable) {
                if (checkCollision(entityRect, grid[r][c].boundingBox)) {
                    return true; // 发生物理碰撞
                }
            }
        }
    }
    return false;
}

bool Block::canPlaceObstacle(int row, int col) const {
    if (row < 0 || row >= totalRows || col < 0 || col >= totalCols) return false;
    const auto& cell = grid[row][col];
    return (cell.type == TileType::Path && !cell.hasObstacle && !cell.hasTower);
}

bool Block::canPlaceTower(int row, int col) const {
    if (row < 0 || row >= totalRows || col < 0 || col >= totalCols) return false;
    const auto& cell = grid[row][col];
    return (cell.type == TileType::Highland && !cell.hasTower);
}

bool Block::placeObstacleAt(int row, int col) {
    if (canPlaceObstacle(row, col)) {
        grid[row][col].hasObstacle = true;
        grid[row][col].isWalkable = false; // 放置障碍物立刻封锁路径
        std::cout << "[Block] Obstacle placed at (" << row << "," << col << "). Path blocked.\n";
        return true;
    }
    return false;
}

bool Block::removeObstacleAt(int row, int col) {
    if (row >= 0 && row < totalRows && col >= 0 && col < totalCols) {
        if (grid[row][col].hasObstacle) {
            grid[row][col].hasObstacle = false;
            grid[row][col].isWalkable = true; // 拆除后恢复通行
            return true;
        }
    }
    return false;
}

bool Block::placeTowerAt(int row, int col) {
    if (canPlaceTower(row, col)) {
        grid[row][col].hasTower = true;
        return true;
    }
    return false;
}
