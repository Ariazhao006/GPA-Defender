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

    // Q0 exercise frequency
    {
        Question q;
        q.prompt = "How often do you exercise each week? ";
        q.options = {
            { "Every day. I feel off if I skip it. ", { { Indicator::Physical, +4 } } },
            { "Three or four times. I move when I can. ", { { Indicator::Physical, +2 } } },
            { "Once or twice. Just enough to count. ", { { Indicator::Physical, +1 } } },
            { "Almost never. Exercise is not happening. ", { { Indicator::Physical, -2 } } },
        };
        questions.push_back(std::move(q));
    }

    // Q1 DDL
    {
        Question q;
        q.prompt = "When deadlines pile up and everything is due, your inner voice says: ";
        q.options = {
            { "Start now. Work hard. Keep pushing. ", { { Indicator::Academic, +2 } } },
            { "This is too much. I need a short phone break. ", { { Indicator::Mental, +2 } } },
        };
        questions.push_back(std::move(q));
    }

    // Q2 未来与成绩
    {
        Question q;
        q.prompt = "When you think about college, the future, and your grades: ";
        q.options = {
            { "I want a stronger next step, so I will push for four years. ", { { Indicator::Academic, +2 } } },
            { "It will work out. I just need to study steadily. ", { { Indicator::Mental, +2 } } },
            { "After twelve years of grind, college should have some fun. ",
              { { Indicator::Mental, +2 }, { Indicator::Academic, -2 } } },
        };
        questions.push_back(std::move(q));
    }

    // Q3 日常生活
    {
        Question q;
        q.prompt = "In daily life, you usually: ";
        q.options = {
            { "Need plenty of time with friends. ", { { Indicator::Connection, +4 } } },
            { "Need private space, but still feel empty with no one around. ",
              { { Indicator::Connection, +1 } } },
            { "Prefer solo mode. Less social noise is better. ",
              { { Indicator::Connection, -2 } } },
        };
        questions.push_back(std::move(q));
    }

    // Q4 受委屈
    {
        Question q;
        q.prompt = "When you feel unfairly blamed: ";
        q.options = {
            { "Maybe I did something wrong. That hurts. ", { { Indicator::Mental, +2 } } },
            { "I did my part. Their blame is their problem. ", { { Indicator::Mental, -2 } } },
        };
        questions.push_back(std::move(q));
    }

    // Q5 期末前一晚
    {
        Question q;
        q.prompt = "The night before finals, you are more likely to: ";
        q.options = {
            { "Keep studying for one last push. ", { { Indicator::Academic, +2 } } },
            { "Protect sleep and avoid an all-nighter. ", { { Indicator::Physical, +2 } } },
            { "Encourage friends and get encouraged back. ", { { Indicator::Connection, +2 } } },
            { "Watch something and clear your head. ", { { Indicator::Mental, +2 } } },
        };
        questions.push_back(std::move(q));
    }

    // Q6 安排被打乱
    {
        Question q;
        q.prompt = "When your whole day gets disrupted: ";
        q.options = {
            { "Reschedule immediately. No time can be wasted. ", { { Indicator::Academic, +2 } } },
            { "Since it is already broken, go exercise. ", { { Indicator::Physical, +2 } } },
            { "Find someone to vent to and chat with. ", { { Indicator::Connection, +2 } } },
            { "The plan is broken, so take a real break first. ", { { Indicator::Mental, +2 } } },
        };
        questions.push_back(std::move(q));
    }

    // Q7 连续低迷一周
    {
        Question q;
        q.prompt = "When you feel low for a whole week: ";
        q.options = {
            { "Study more and rebuild a sense of progress. ", { { Indicator::Academic, +2 } } },
            { "Spend more time with friends. ", { { Indicator::Connection, +2 } } },
            { "Rebuild routine through exercise and sleep. ", { { Indicator::Physical, +2 } } },
            { "Give yourself a break and reset. ", { { Indicator::Mental, +2 } } },
        };
        questions.push_back(std::move(q));
    }

    // Q8 低分作业
    {
        Question q;
        q.prompt = "When an assignment score is lower than expected: ";
        q.options = {
            { "Review mistakes immediately and recover next time. ", { { Indicator::Academic, +2 } } },
            { "Run it off and clear the frustration. ",
              { { Indicator::Physical, +2 }, { Indicator::Mental, +2 } } },
            { "Complain to friends for a while, then deal with it. ",
              { { Indicator::Connection, +2 }, { Indicator::Mental, +2 } } },
            { "Tell yourself it is one score, not your whole identity. ", { { Indicator::Mental, +2 } } },
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

    if (r.rawAcademic == maxRaw) r.tags.push_back("Academic Grinder ");
    if (r.rawMental == maxRaw) r.tags.push_back("Balanced Survivor ");
    if (r.rawPhysical == maxRaw) r.tags.push_back("Fitness Defender ");
    if (r.rawConnection == maxRaw) r.tags.push_back("Social Anchor ");

    return r;
}
