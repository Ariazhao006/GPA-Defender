#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <random>

#include <string>

#include "gpa_defender/Enemy.h"
#include "gpa_defender/Vector2D.h"

namespace frontend {

enum class ChestType {
    Memory,     // 回忆模式：防御塔攻击力下降
    Hell,       // 地狱模式：额外Boss来袭
    Reward,     // 奖励模式：金币++，指标++
    Gamble      // 博弈小游戏：翘课vs点名
};

enum class ChestState {
    Hidden,     // 未出现
    Active,     // 出现在路径上，等待防御塔打掉
    Opened,     // 已开启
    Expired     // 已过期
};

struct Chest {
    Vector2D position;
    ChestType type;
    ChestState state = ChestState::Hidden;
    float timer = 0.0f;         // 存在倒计时
    bool rewardCollected = false;
    bool armedForAttack = false;
    std::unique_ptr<TreasureChestEnemy> target;

    // 渲染相关
    float bounceOffset = 0.0f;
    float bounceTime = 0.0f;
};

class ChestManager {
public:
    ChestManager();

    // 每帧更新
    void update(float deltaTime, int currentWave, int totalWaves);

    // 尝试在指定位置生成宝箱
    void trySpawnChest(const Vector2D& position, int currentWave);
    void trySpawnChestOnPath(const std::vector<std::vector<Vector2D>>& paths, int currentWave);

    // 玩家点击宝箱
    bool tryOpenChest(const Vector2D& clickPos, float clickRadius);

    // 获取当前活跃的宝箱
    const std::vector<Chest>& getActiveChests() const { return chests; }
    std::vector<Enemy*> getLiveTargets() const;

    // 检查是否有回忆模式效果正在生效
    bool isMemoryEffectActive() const;

    // 获取回忆模式的攻击力倍率 (0.5 = 攻击力减半)
    float getAttackMultiplier() const;

    // 获取待触发的地狱模式事件（调用后清除）
    bool hasHellEvent() const;
    void clearHellEvent();

    // 获取待触发的奖励事件（调用后清除）
    bool hasRewardEvent() const;
    int getRewardGold() const;
    void clearRewardEvent();

    // 获取博弈结果（调用后清除）
    bool hasGambleResult() const;
    bool getGambleWon() const;
    int getGambleGold() const;
    void clearGambleResult();

    // 获取最后触发的效果文本（调用后清除）
    bool hasEffectMessage() const { return effectDisplayTimer > 0.0f; }
    const std::string& getEffectMessage() const { return effectMessage; }
    void clearEffectMessage();

    // 重置
    void reset();

private:
    std::vector<Chest> chests;

    // 事件标记
    bool hellEventPending = false;
    bool rewardEventPending = false;
    int rewardGold = 0;
    bool gambleResultPending = false;
    bool gambleWon = false;
    int gambleGold = 0;
    std::string effectMessage;
    float effectDisplayTimer = 0.0f;
    static constexpr float kEffectDisplayDuration = 3.0f;

    // 回忆模式效果
    float memoryEffectTimer = 0.0f;
    static constexpr float kMemoryEffectDuration = 10.0f;
    static constexpr float kMemoryAttackMultiplier = 0.5f;

    // 随机数生成
    std::mt19937 rng;

    // 生成随机宝箱类型
    ChestType randomChestType();

    // 设置效果消息（同时启动显示计时器）
    void setEffectMessage(const std::string& msg);

    // 清理过期宝箱
    void cleanupExpiredChests();
};

} // namespace frontend
