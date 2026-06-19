# ASTI 模块说明（后端）

本文说明「预开始问卷 → 人格标签与四项阈值 → 写入玩家状态」的后端实现，供主循环、前端、怪物/运动模式相关同学对接。

## 1. 涉及哪些文件


| 文件                                                                                                                                    | 作用                                               |
| ------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------ |
| `[Indicator.h](d:/vscodecppfiles/GPA defender/Indicator.h)`                                                                           | 四项指标枚举：`学业 / 身体 / 心理 / 联结`                       |
| `[Questionnaire.h](d:/vscodecppfiles/GPA defender/Questionnaire.h)`                                                                   | 题目结构体、`AstiResult`、`Questionnaire` 类声明           |
| `[Questionnaire.cpp](d:/vscodecppfiles/GPA defender/Questionnaire.cpp)`                                                               | 题库数据、`score()` 评分与标签判定                           |
| `[PlayerStats.h](d:/vscodecppfiles/GPA defender/PlayerStats.h)` / `[PlayerStats.cpp](d:/vscodecppfiles/GPA defender/PlayerStats.cpp)` | 玩家四项当前值、阈值、ASTI 标签；`applyAstiResult`、`isAlive` 等 |
| `[ASTI模块说明.md](d:/vscodecppfiles/GPA defender/ASTI模块说明.md)`                                                                           | 本文档                                              |


问卷文案与加减分规则与 `[游戏内容设计.md](d:/vscodecppfiles/GPA defender/游戏内容设计.md)` 中「预开始——测测你的 ASTI」一节一致。

## 2. 策划规则在代码里怎么落地

### 2.1 加号与整数

设计文档里用「+、++、++++、--」表示力度，在 `[Questionnaire.cpp](d:/vscodecppfiles/GPA defender/Questionnaire.cpp)` 里已**直接写成整数**，对应关系为：

- `+` → +1  
- `++` → +2  
- `++++` → +4  
- `--` → -2

多指标同一选项时，在 `effects` 里写多条 `OptionEffect`。

### 2.2 原始分 → 阈值

每答完一题，只累加 **raw**（原始分）。全部答完后：

- `threshold = 50 + raw * 2`  
- 再限制在 **[35, 90]** 区间内，避免数值爆炸或过低。

四项独立计算，互不影响。

### 2.3 人格标签（ASTI Type）

在四个 **raw** 中找出**最大值** `maxRaw`：

- 学业 raw == maxRaw → 标签 **「极限卷王」**
- 心理 raw == maxRaw → **「随缘活着」**
- 身体 raw == maxRaw → **「健身狂魔」**
- 联结 raw == maxRaw → **「社交达人」**

若多项并列最高，则**多个标签同时输出**（与设计文档「并列就并列输出」一致）。

> 因四项阈值使用同一线性公式，raw 最大的一项对应的 threshold 通常也是最高；若日后改公式，仍以「raw 并列」判定标签即可与设计意图对齐。

## 3. 问卷要不要用「很多类」？

**不需要。** 实现方式是：

- 用 `struct Question / QuestionOption / OptionEffect` 描述数据；
- 用 `class Questionnaire` 装 `vector<Question>`，并提供 `score(answers)`。

没有复杂的继承；前端只传**每道题选了第几个选项**即可。

## 4. PlayerStats：本次写到哪里、哪里留给队友

### 4.1 已实现（ASTI 相关）

- `applyAstiResult(const AstiResult&)`：把问卷算出的**四项阈值**和**标签**写入对象；并把**当前值**设为 `阈值 + 25`（封顶 100），避免开局就贴线失败。
- `isAlive()`：当且仅当四项**当前值均 ≥ 各自阈值**时返回 `true`，供通关/存活判定。
- `getThreshold`* / `getCurrent*` / `getAstiTags()`：给 UI 与调试打印。

### 4.2 仅接口、逻辑由队友补全

- `changeAcademic` / `changePhysical` / `changeMental` / `changeConnection`：对当前值做加减并夹在 [0,100]。**漏怪、事件、宝箱等**应通过这些接口改指标，不要直接改私有成员。
- `setExerciseMode(bool)` / `getExerciseMode()`：**只保存开关**。设计文档要求：
  - 开启运动模式：身体指标应上升、防御塔攻击力下降等——需主循环每帧或塔模块读取 `getExerciseMode()` 后实现；
  - 未开启时身体匀速下降——同样由主循环在每帧调用 `changePhysical` 实现。

本模块**不负责**「每帧身体自然变化」的具体数值，避免与塔攻击力逻辑耦合在一处。

## 5. 主循环同学怎么用

### 5.1 开局流程（推荐）

1. 前端收集 9 个整数答案 `std::vector<int> answers`（长度必须等于 `questionCount()`）。
2. `Questionnaire q = buildAstiQuestionnaire();`
3. `AstiResult r = q.score(answers);`
4. `PlayerStats player; player.applyAstiResult(r);`
5. 进入关卡循环后，每帧或每事件里更新 `player` 的当前值，并在适当时机判断 `player.isAlive()`。

### 5.2 合法性检查

- 调用 `score` 前请确认 `answers.size() == q.questionCount()`（当前为 **9**）。
- 若某题下标越界，`score` 会返回 **tags 为空、四项阈值为 50** 的中性结果；主循环应拒绝进入战斗或提示重新答题。

## 6. 前端同学怎么用

- 按 `Questionnaire::getQuestions()` 或自行与策划表对齐，展示题干与选项。
- 提交时用 **0 起始下标** 表示每题所选选项。
- 展示结果：使用 `AstiResult::tags` 与四项 `threshold`*；战斗中 HUD 用 `PlayerStats` 的 `getCurrent*` 与 `getThreshold*`。

## 8. 自测入口

`[main.cpp](d:/vscodecppfiles/GPA defender/main.cpp)` 开头有一段 **ASTI 自测**：使用「每题都选第 0 个选项」的演示答案，打印 raw、阈值、标签、开局当前值与 `isAlive()`。编译命令见 `[.vscode/tasks.json](d:/vscodecppfiles/GPA defender/.vscode/tasks.json)`（已加入 `Questionnaire.cpp`、`PlayerStats.cpp`）。