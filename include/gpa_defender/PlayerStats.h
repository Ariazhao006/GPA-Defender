#pragma once
#ifndef PLAYER_STATS_H
#define PLAYER_STATS_H

#include <string>
#include <vector>
#include "gpa_defender/Questionnaire.h"

/**
 * @class PlayerStats
 * @brief 玩家（小鲤）存活指标与 ASTI 结果
 *
 * 本类本次只完整实现「问卷 -> AstiResult -> 写入阈值与标签、开局当前值」；
 * 运动模式每帧回血、未开启时身体匀速下降、怪物漏怪扣指标等，请由主循环与
 * 敌人模块同学调用下方 setter / change 接口实现。
 */
class PlayerStats {
private:
    // 当前值（战斗中会波动）
    int currentAcademic = 0;
    int currentPhysical = 0;
    int currentMental = 0;
    int currentConnection = 0;

    // 通关判定用：各项当前值需 >= 对应阈值
    int thresholdAcademic = 0;
    int thresholdPhysical = 0;
    int thresholdMental = 0;
    int thresholdConnection = 0;

    std::vector<std::string> astiTags;

    /** 是否开启运动模式（仅存状态；具体效果见 ASTI模块说明.md） */
    bool exerciseMode = false;

    static int clampInt(int v, int lo, int hi);

public:
    PlayerStats() = default;

    /**
     * 将问卷评分结果写入本对象：阈值、标签、当前值（在阈值之上留出缓冲）
     */
    void applyAstiResult(const AstiResult& result);

    /** 四项当前值均不低于各自阈值时视为仍存活（可用于关卡结束判定） */
    bool isAlive() const;

    void setExerciseMode(bool on);
    bool getExerciseMode() const { return exerciseMode; }

    void changeAcademic(int delta);
    void changePhysical(int delta);
    void changeMental(int delta);
    void changeConnection(int delta);

    int getCurrentAcademic() const { return currentAcademic; }
    int getCurrentPhysical() const { return currentPhysical; }
    int getCurrentMental() const { return currentMental; }
    int getCurrentConnection() const { return currentConnection; }

    int getThresholdAcademic() const { return thresholdAcademic; }
    int getThresholdPhysical() const { return thresholdPhysical; }
    int getThresholdMental() const { return thresholdMental; }
    int getThresholdConnection() const { return thresholdConnection; }

    const std::vector<std::string>& getAstiTags() const { return astiTags; }
};

#endif // PLAYER_STATS_H

