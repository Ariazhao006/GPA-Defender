#include "gpa_defender/Questionnaire.h"
#include <algorithm>

namespace {

    // 将原始分线性映射为阈值，并限制在 [minTh, maxTh]
    int mapRawToThreshold(int raw, int base = 50, int scale = 2, int minTh = 35, int maxTh = 90) {
        int t = base + raw * scale;
        if (t < minTh) t = minTh;
        if (t > maxTh) t = maxTh;
        return t;
    }

    void addRaw(AstiResult& r, Indicator ind, int delta) {
        switch (ind) {
        case Indicator::Academic:   r.rawAcademic += delta; break;
        case Indicator::Physical:   r.rawPhysical += delta; break;
        case Indicator::Mental:     r.rawMental += delta; break;
        case Indicator::Connection: r.rawConnection += delta; break;
        }
    }

} // namespace

Questionnaire::Questionnaire() {
    questions.clear();

    // Q0 运动频率（+ =1, ++=2, ++++=4, --=2 扣分取负）
    {
        Question q;
        q.prompt = "你每周运动的频率是？ "; // 注意末尾加了空格，下同
        q.options = {
            { "每天。 \n一天不运动我浑身难受 ", { { Indicator::Physical, +4 } } },
            { "3-4次。 \n有空就动 ", { { Indicator::Physical, +2 } } },
            { "1-2次。差不多动一下 ", { { Indicator::Physical, +1 } } },
            { "无。运动不了一点 ", { { Indicator::Physical, -2 } } },
        };
        questions.push_back(std::move(q));
    }

    // Q1 DDL
    {
        Question q;
        q.prompt = "如果这一段时间，你有非常多ddl，非常多要做的事情，你的内心os： ";
        q.options = {
            { "啊啊啊，那我要赶快开始做，努力做，拼命做 ", { { Indicator::Academic, +2 } } },
            { "压力这么大，那我摆一会好了，手机启动 ", { { Indicator::Mental, +2 } } },
        };
        questions.push_back(std::move(q));
    }

    // Q2 未来与成绩
    {
        Question q;
        q.prompt = "上大学后，你想到未来，又想到自己的成绩： ";
        q.options = {
            { "我要去更好的学校，拼搏四年，更上一层楼！ ", { { Indicator::Academic, +2 } } },
            { "其实到头来都一样，认真学学就好 ", { { Indicator::Mental, +2 } } },
            { "都寒窗苦读12年了，上大学还不玩，更待何时 ",
              { { Indicator::Mental, +2 }, { Indicator::Academic, -2 } } },
        };
        questions.push_back(std::move(q));
    }

    // Q3 日常生活
    {
        Question q;
        q.prompt = "日常生活中，你： ";
        q.options = {
            { "我是友宝女，我需要和很多朋友贴贴 ", { { Indicator::Connection, +4 } } },
            { "我需要一定的私人空间，但如果一直没有朋友和我说话，我也会心里空唠唠的 ",
              { { Indicator::Connection, +1 } } },
            { "额啊，我不要社交，solo trip才是最好的地球online打开方式 ",
              { { Indicator::Connection, -2 } } },
        };
        questions.push_back(std::move(q));
    }

    // Q4 受委屈
    {
        Question q;
        q.prompt = "当你受委屈时： ";
        q.options = {
            { "唉，是不是我哪里做的不够好，好伤心 ", { { Indicator::Mental, +2 } } },
            { "我已经做得很好了，他们指责我是他们的问题 ", { { Indicator::Mental, -2 } } },
        };
        questions.push_back(std::move(q));
    }

    // Q5 期末前一晚
    {
        Question q;
        q.prompt = "期末考试前一晚，你更可能： ";
        q.options = {
            { "继续冲刺学习 ", { { Indicator::Academic, +2 } } },
            { "保证睡眠，不熬通宵 ", { { Indicator::Physical, +2 } } },
            { "和朋友互相打气 ", { { Indicator::Connection, +2 } } },
            { "看会儿视频放空 ", { { Indicator::Mental, +2 } } },
        };
        questions.push_back(std::move(q));
    }

    // Q6 安排被打乱
    {
        Question q;
        q.prompt = "一天安排被打乱时： ";
        q.options = {
            { "立刻重新安排计划，不能浪费时间 ", { { Indicator::Academic, +2 } } },
            { "事已至此，去跑步（运动）得了 ", { { Indicator::Physical, +2 } } },
            { "找人吐槽，顺便聊聊天 ", { { Indicator::Connection, +2 } } },
            { "都被打乱了，那先休息吧 ", { { Indicator::Mental, +2 } } },
        };
        questions.push_back(std::move(q));
    }

    // Q7 连续低迷一周
    {
        Question q;
        q.prompt = "当你连续一周状态都很低迷时： ";
        q.options = {
            { "增加学习投入，找到成就感 ", { { Indicator::Academic, +2 } } },
            { "多和朋友见面 ", { { Indicator::Connection, +2 } } },
            { "规律锻炼和作息 ", { { Indicator::Physical, +2 } } },
            { "给自己放假，放空 ", { { Indicator::Mental, +2 } } },
        };
        questions.push_back(std::move(q));
    }

    // Q8 低分作业
    {
        Question q;
        q.prompt = "当你拿到一份比预期低的作业分数： ";
        q.options = {
            { "立刻分析错题，下次拉回来 ", { { Indicator::Academic, +2 } } },
            { "出去跑两圈把情绪甩掉 ",
              { { Indicator::Physical, +2 }, { Indicator::Mental, +2 } } },
            { "找朋友吐槽一下午，再说 ",
              { { Indicator::Connection, +2 }, { Indicator::Mental, +2 } } },
            { "告诉自己一次而已，别 PUA 自己 ", { { Indicator::Mental, +2 } } },
        };
        questions.push_back(std::move(q));
    }
}

Questionnaire buildAstiQuestionnaire() {
    return Questionnaire();
}

AstiResult Questionnaire::score(const std::vector<int>& answers) const {
    AstiResult r;

    if (answers.size() != questions.size()) {
        r.thresholdAcademic = r.thresholdPhysical = r.thresholdMental = r.thresholdConnection = 50;
        return r;
    }

    for (std::size_t i = 0; i < questions.size(); ++i) {
        int idx = answers[i];
        if (idx < 0 || static_cast<std::size_t>(idx) >= questions[i].options.size()) {
            r.tags.clear();
            r.rawAcademic = r.rawPhysical = r.rawMental = r.rawConnection = 0;
            r.thresholdAcademic = r.thresholdPhysical = r.thresholdMental = r.thresholdConnection = 50;
            return r;
        }
        for (const OptionEffect& eff : questions[i].options[static_cast<std::size_t>(idx)].effects) {
            addRaw(r, eff.indicator, eff.delta);
        }
    }

    r.thresholdAcademic = mapRawToThreshold(r.rawAcademic);
    r.thresholdPhysical = mapRawToThreshold(r.rawPhysical);
    r.thresholdMental = mapRawToThreshold(r.rawMental);
    r.thresholdConnection = mapRawToThreshold(r.rawConnection);

    const int maxRaw = std::max({ r.rawAcademic, r.rawPhysical, r.rawMental, r.rawConnection });

    if (r.rawAcademic == maxRaw) r.tags.push_back("极限卷王 "); // 加了空格
    if (r.rawMental == maxRaw) r.tags.push_back("随缘活着 "); // 加了空格
    if (r.rawPhysical == maxRaw) r.tags.push_back("健身狂魔 "); // 加了空格
    if (r.rawConnection == maxRaw) r.tags.push_back("社交达人 "); // 加了空格

    return r;
}
