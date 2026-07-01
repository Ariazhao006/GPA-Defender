# GPA Defender - Backend Implementation

## 概述

这份文档描述当前后端结构在工程化初始化后的落点。仓库已经切到 CMake 主入口，源码和测试不再堆在根目录。

## 当前构建方式

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

如果本机 `cmake` 不在 PATH 中，可以先设置 `CMAKE_EXE`，或者直接使用 `scripts/build_and_test.bat` / `scripts/build_and_test.sh`。

## 当前测试入口

- `gpa_defender_app`：主程序入口
- `backend_smoke`：轻量 smoke test
- `full_flow_smoke`：完整流程 smoke test

## 当前目录约定

```text
include/gpa_defender/   # 头文件
src/                    # 核心实现与场景入口
tests/                  # smoke test 源码
docs/                   # 设计文档与说明
scripts/                # 构建辅助脚本
assets/                 # PDF 等静态资源
```

## 说明

- `src/full_flow_scenario.cpp` 承载原来的完整流程演示逻辑。
- `tests/BackendLoopSmokeTest.cpp` 保留轻量后端流程检查。
- `GameUI` 仍在仓库中，但当前 smoke test 不依赖交互式 UI 文件。

---

## 🔧 API 速查表

### GameEngine - 初始化与控制

```cpp
// 初始化（从 ASTI 问卷结果）
engine.initializeFromAsti(astiResult);

// 启动波次
engine.startWave();

// 主循环更新（每帧调用）
engine.update(deltaTime);  // 单位: 秒

// 购买塔
bool ok = engine.tryBuyTower(TowerKind::Coffee, position);

// 升级塔
bool ok = engine.tryUpgradeAiTower(towerIndex);

// 运动模式
engine.setExerciseMode(true);   // 启用
engine.setExerciseMode(false);  // 禁用
bool isOn = engine.getExerciseMode();

// 查询状态
GamePhase phase = engine.getPhase();
int gold = engine.getGold();
int waveIdx = engine.getWaveIndex();
GameSnapshot snap = engine.getSnapshot();
```

### WaveManager - 波次与敌人

```cpp
// 波次定义
struct WaveDefinition {
    std::vector<SpawnEvent> spawns;  // 敌人生成计划
    int clearBonus;                  // 清波奖励
};

// 敌人生成事件
struct SpawnEvent {
    float timeSec;        // 生成时间（秒）
    EnemyKind kind;       // 敌人类型
    int pathId;           // 路径 ID
};

// 查询方法
int activeCount = waveManager.getActiveEnemyCount();
int totalSpawns = waveManager.getTotalSpawnCount();
bool cleared = waveManager.isWaveCleared();
```

### GameSnapshot - 状态快照

```cpp
GameSnapshot snap = engine.getSnapshot();

// 游戏状态
snap.phase;             // 当前相位
snap.gold;              // 当前金币
snap.waveIndex;         // 当前波次（从 0 开始）
snap.exerciseMode;      // 是否运动中

// 四维指标（当前值）
snap.currentAcademic;
snap.currentPhysical;
snap.currentMental;
snap.currentConnection;

// 四维指标（阈值）
snap.thresholdAcademic;
snap.thresholdPhysical;
snap.thresholdMental;
snap.thresholdConnection;

// 波次统计
snap.activeEnemies;     // 活跃敌人数
snap.totalWaveSpawns;   // 本波计划敌人数
snap.waveTimeSec;       // 波次经过时间
```

---

## 🐛 已知问题与改进方向

### ✅ 本周期已修复

- [x] Enemy.h include guard 提前结束（移动到文件末尾）
- [x] `applySlowEffect` 改为虚函数（支持子类重写）
- [x] 编译配置与 GameEngine/WaveManager 集成（tasks.json）

### ⏳ 后续改进计划

- [ ] **波次外部化**: JSON/CSV 配置替代硬编码
- [ ] **固定时间步**: 确保不同帧率下逻辑一致
- [ ] **事件系统**: 观察者模式解耦前端
- [ ] **单元测试**: 集成 Google Test/Catch2
- [ ] **对象池**: 减少频繁分配释放开销
- [ ] **REST API**: 前后端接口定义
- [ ] **序列化**: save/load 功能

详见 [IMPLEMENTATION_GUIDE.md](./IMPLEMENTATION_GUIDE.md#后续改进计划)

---

## 📞 常见问题

### Q: 如何修改波次配置？

**A**: 目前在 `GameEngine::defaultWaves()` 中硬编码。后续建议迁移到 JSON 文件。

### Q: 运动模式对游戏难度的影响？

**A**: 
- **启用**: 身体恢复但塔攻击频率降低（DPS ≈ 65%）
- **禁用**: 身体衰减但塔可正常输出

是一个平衡权衡。

### Q: 如何自定义敌人伤害？

**A**: Enemy 类定义了 `dmgAcademic/Physical/Mental/Connection`，由 `update()` 方法调用。修改这些值即可改变伤害。

### Q: 是否支持多人游戏？

**A**: 目前不支持。单机逻辑完善后，可通过 REST API 或 WebSocket 扩展为网络版。

### Q: 能否集成到前端（Web/Unity）？

**A**: 目前是纯 C++ 后端。需要：
1. 实现 JSON 序列化
2. 创建 REST API 网关 或 C++ 绑定 (SWIG/binding)
3. 前端通过 HTTP 或直接调用库

---

## 🤝 贡献指南

欢迎提交 Issue 和 Pull Request！

### 代码规范

- 类名: `PascalCase`
- 函数名: `camelCase`
- 常量: `kCamelCase`
- 缩进: 4 个空格
- 行长: ≤ 100 字符

### 提交要求

- 通过编译 (`g++ -std=c++17 -fsyntax-only`)
- 通过全流程测试 (`./main.exe`)
- 通过 UI 测试 (`./ui_test.exe`)
- 补充相应文档和注释

---

## 📄 许可证

[待定] - 请参考项目许可证

---

## 🎓 学习资源

- [C++ 官方标准](https://en.cppreference.com/)
- [设计模式](https://en.cppreference.com/w/cpp/language/raii)
- [游戏架构](https://www.gameprogrammingpatterns.com/)

---

## 📞 联系方式

- **项目负责**: Backend Development Team
- **主要贡献者**: [待补充]
- **相关文档**: 
  - [IMPLEMENTATION_GUIDE.md](./IMPLEMENTATION_GUIDE.md)
  - [PROJECT_AUDIT_REPORT.md](./PROJECT_AUDIT_REPORT.md)

---

**最后更新**: 2026-05-21  
**版本**: 1.0 (Stable)

Happy coding! 🚀
