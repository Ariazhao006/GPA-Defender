# GPA Defender

GPA Defender 是一个 C++17 塔防原型项目。当前仓库以 CMake 作为标准构建入口，并把核心逻辑、可执行入口、测试、文档和脚本拆分到了各自目录，方便继续开发和接入工具链。

## 目录结构

```text
.
├── assets/                  # PDF 和其他静态资源
├── docs/                    # 设计说明、后端说明、spec / plan
├── include/gpa_defender/    # 项目头文件
├── scripts/                 # 构建与测试脚本
├── src/                     # 核心实现与主程序入口
├── tests/                   # smoke test 入口与测试源码
├── .vscode/                 # VS Code 任务与调试配置
├── CMakeLists.txt
└── .gitignore
```

## 前置要求

- CMake 3.20+
- 支持 C++17 的编译器
- Windows 下当前已验证 `g++` 可用；VS Code 调试配置默认使用 `gdb`

## 构建

```bash
cmake -S . -B build
cmake --build build
```

如果 `cmake` 不在 PATH 中，可以先设置 `CMAKE_EXE` 指向对应可执行文件，再运行 `scripts/build_and_test.bat` 或 `scripts/build_and_test.sh`。

## 测试

```bash
ctest --test-dir build --output-on-failure
```

当前注册了两个 smoke tests：

- `backend_smoke`
- `full_flow_smoke`

## VS Code

- 默认构建任务：`CMake: build`
- 测试任务：`CMake: test`
- 调试入口：`C/C++: Run gpa_defender_app`

## 当前说明

- `src/main.cpp` 只保留主程序入口。
- `src/full_flow_scenario.cpp` 承载完整流程场景，供主程序和 smoke test 复用。
- 旧的根目录 `.exe` 产物不再作为仓库内容的一部分。
