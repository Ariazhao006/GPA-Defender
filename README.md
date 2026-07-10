# GPA Defender

GPA Defender 是一个使用 C++17 和 raylib 开发的塔防游戏项目。玩家需要放置不同类型的防御塔，抵御一波波敌人，并尽量维持 GPA 与各项状态指标。

## 如何运行

### 方法一：直接运行脚本（推荐 Windows）

在项目根目录双击：

```text
run.bat
```

脚本会自动编译并启动游戏。第一次运行可能需要等待 raylib 和项目代码完成编译。

如果游戏窗口已经打开，再次运行可能会编译失败；请先关闭游戏窗口后重试。

### 方法二：命令行运行

在项目根目录打开终端，执行：

```bash
cmake -S . -B build
cmake --build build --target gpa_defender_gui
```

编译完成后运行：

```text
build/bin/gpa_defender_gui.exe
```

### 方法三：使用 Visual Studio / VS Code

也可以用支持 CMake 的 IDE 打开项目根目录，然后选择并运行目标：

```text
gpa_defender_gui
```

## 怎么玩

1. 进入游戏后点击 `New Game` 开始新游戏，按流程完成问卷和关卡选择。
2. 在建造阶段选择右侧的防御塔，然后点击地图上的可放置位置进行建造。
3. 点击 `Start Wave` 开始敌人进攻。敌人会沿路线移动，防御塔会自动攻击。
4. 击败敌人可以获得金币，金币可用于建造、升级或调整防御塔。
5. 游戏中会随机出现宝箱。点击宝箱后，防御塔会攻击宝箱；宝箱被击破后会触发奖励、挑战或小游戏事件。
6. 目标是在所有波次结束前保护 GPA，并让各项状态指标保持在安全范围内。

## 运行环境

- CMake 3.20 或以上
- 支持 C++17 的编译器
- Windows 推荐使用 Visual Studio 或 MSYS2/MinGW

## 项目入口

- GUI 游戏入口：`src/gui_main.cpp`
- 核心后端逻辑：`src/`
- 头文件：`include/gpa_defender/`
- 游戏素材：`assets/`

运行时请保留 `assets` 文件夹，否则图片、音频或视频资源可能无法正常加载。
