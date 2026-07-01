#include "gpa_defender/PlayerStats.h"

namespace {

// 开局时在阈值之上额外给出的缓冲，避免一上来就贴线失败
constexpr int kStartBufferAboveThreshold = 25;
constexpr int kStatMin = 0;
constexpr int kStatMax = 100;

} // namespace

int PlayerStats::clampInt(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void PlayerStats::applyAstiResult(const AstiResult& result) {
    thresholdAcademic = result.thresholdAcademic;
    thresholdPhysical = result.thresholdPhysical;
    thresholdMental = result.thresholdMental;
    thresholdConnection = result.thresholdConnection;

    astiTags = result.tags;

    currentAcademic = clampInt(thresholdAcademic + kStartBufferAboveThreshold, kStatMin, kStatMax);
    currentPhysical = clampInt(thresholdPhysical + kStartBufferAboveThreshold, kStatMin, kStatMax);
    currentMental = clampInt(thresholdMental + kStartBufferAboveThreshold, kStatMin, kStatMax);
    currentConnection = clampInt(thresholdConnection + kStartBufferAboveThreshold, kStatMin, kStatMax);
}

bool PlayerStats::isAlive() const {
    return currentAcademic >= thresholdAcademic
        && currentPhysical >= thresholdPhysical
        && currentMental >= thresholdMental
        && currentConnection >= thresholdConnection;
}

void PlayerStats::setExerciseMode(bool on) {
    exerciseMode = on;
    // 设计说明：开启时身体指标应随时间上升、防御塔攻击力下降等，由主循环与塔模块实现。
    // 此处仅保存开关状态，便于队友读取 getExerciseMode()。
}

void PlayerStats::changeAcademic(int delta) {
    currentAcademic = clampInt(currentAcademic + delta, kStatMin, kStatMax);
}

void PlayerStats::changePhysical(int delta) {
    currentPhysical = clampInt(currentPhysical + delta, kStatMin, kStatMax);
}

void PlayerStats::changeMental(int delta) {
    currentMental = clampInt(currentMental + delta, kStatMin, kStatMax);
}

void PlayerStats::changeConnection(int delta) {
    currentConnection = clampInt(currentConnection + delta, kStatMin, kStatMax);
}

