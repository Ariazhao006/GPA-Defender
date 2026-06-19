#pragma once
#ifndef INDICATOR_H
#define INDICATOR_H

/**
 * 人物四项存活指标（与《游戏内容设计》一致）
 *
 * 问卷加分、怪物伤害、通关判定均围绕这四项。
 */
enum class Indicator {
    Academic,   // 学业成绩
    Physical,   // 身体健康
    Mental,     // 心理健康
    Connection  // 联结感
};

#endif // INDICATOR_H

