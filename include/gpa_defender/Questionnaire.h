#pragma once
#ifndef QUESTIONNAIRE_H
#define QUESTIONNAIRE_H

#include <string>
#include <vector>
#include "gpa_defender/Indicator.h"

/** 单个选项对某一指标的分数修正 */
struct OptionEffect {
    Indicator indicator;
    int delta; // 正为加分，负为扣分
};

/** 问卷中某一题的单个选项 */
struct QuestionOption {
    std::string text;                    // 选项展示文案
    std::vector<OptionEffect> effects;   // 可多指标同时变化
};

/** 问卷中的一道题 */
struct Question {
    std::string prompt;                  // 题干
    std::vector<QuestionOption> options; // 选项列表（下标与前端传入一致，从 0 开始）
};

/**
 * ASTI 问卷评分结果：
 * - raw*：各指标原始累计分（仅问卷贡献）
 * - threshold*：换算后的存活阈值（用于与当前值比较）
 * - tags：人格标签，与 raw 四项中「并列最高分」对应，可多个并存
 */
struct AstiResult {
    int rawAcademic = 0;
    int rawPhysical = 0;
    int rawMental = 0;
    int rawConnection = 0;

    int thresholdAcademic = 0;
    int thresholdPhysical = 0;
    int thresholdMental = 0;
    int thresholdConnection = 0;

    std::vector<std::string> tags;
};

/**
 * 持有整套 ASTI 题目，并提供根据作答计算 AstiResult 的接口。
 * 题库内容来自《游戏内容设计》预开始问卷。
 */
class Questionnaire {
private:
    std::vector<Question> questions;

public:
    Questionnaire();

    /** 题目总数，应与前端传入的答案向量长度一致（当前为 9） */
    std::size_t questionCount() const { return questions.size(); }

    const std::vector<Question>& getQuestions() const { return questions; }

    /**
     * @param answers 每道题所选选项下标，answers[i] 表示第 i 题选第几个选项（从 0 开始）
     * @return 评分结果；若长度不符或某下标非法，tags 为空且阈值为中性默认值 50
     */
    AstiResult score(const std::vector<int>& answers) const;
};

/** 构造与《游戏内容设计》一致的默认题库 */
Questionnaire buildAstiQuestionnaire();

#endif // QUESTIONNAIRE_H

