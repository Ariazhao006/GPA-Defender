# GPA Defender Project Initialization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reorganize the existing GPA Defender C++ project into a CMake-first, developer-ready repository with standard directories, runnable smoke tests, and aligned docs/editor tooling.

**Architecture:** Keep gameplay behavior unchanged while separating reusable game logic into a `gpa_defender_core` static library, a main application executable, and smoke-test executables wired through CTest. Migrate files into `include/gpa_defender`, `src`, `tests`, `docs`, `scripts`, and `assets`, then update editor and helper entrypoints to use the same CMake build graph.

**Tech Stack:** C++17, CMake, CTest, VS Code C/C++ tooling, Windows batch, POSIX shell

---

## Planned File Structure

- Create: `D:\Projects\GPA defender\CMakeLists.txt`
- Create: `D:\Projects\GPA defender\README.md`
- Create: `D:\Projects\GPA defender\.gitignore`
- Create: `D:\Projects\GPA defender\tests\backend_smoke_main.cpp`
- Create: `D:\Projects\GPA defender\tests\full_flow_smoke_main.cpp`
- Create: `D:\Projects\GPA defender\src\full_flow_scenario.h`
- Create: `D:\Projects\GPA defender\src\full_flow_scenario.cpp`
- Modify: `D:\Projects\GPA defender\.vscode\tasks.json`
- Modify: `D:\Projects\GPA defender\.vscode\launch.json`
- Modify: `D:\Projects\GPA defender\scripts\build_and_test.bat`
- Modify: `D:\Projects\GPA defender\scripts\build_and_test.sh`
- Move: `D:\Projects\GPA defender\*.h` to `D:\Projects\GPA defender\include\gpa_defender\`
- Move: `D:\Projects\GPA defender\*.cpp` implementation files to `D:\Projects\GPA defender\src\`
- Move: `D:\Projects\GPA defender\BackendLoopSmokeTest.cpp` to `D:\Projects\GPA defender\tests\BackendLoopSmokeTest.cpp`
- Move: `D:\Projects\GPA defender\README_BACKEND.md` and Chinese markdown docs to `D:\Projects\GPA defender\docs\`
- Move: `D:\Projects\GPA defender\塔防游戏 Proposal.pdf` to `D:\Projects\GPA defender\assets\塔防游戏 Proposal.pdf`

### Task 1: Establish the CMake Baseline

**Files:**
- Create: `D:\Projects\GPA defender\CMakeLists.txt`
- Create: `D:\Projects\GPA defender\.gitignore`
- Test: repository root CMake configure

- [ ] **Step 1: Run the current CMake configure command to prove the baseline is missing**

Run: `cmake -S . -B build`
Expected: FAIL with a message equivalent to `does not appear to contain CMakeLists.txt`

- [ ] **Step 2: Add the top-level CMake project and ignore rules**

```cmake
cmake_minimum_required(VERSION 3.20)
project(GPADefender LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

enable_testing()
```

```gitignore
/build/
*.exe
*.obj
*.o
*.pdb
*.ilk
*.stackdump
.DS_Store
Thumbs.db
```

- [ ] **Step 3: Re-run CMake configure to verify the project now configures**

Run: `cmake -S . -B build`
Expected: PASS with `Configuring done` and `Generating done`

- [ ] **Step 4: Commit the baseline build entrypoint**

```bash
git add CMakeLists.txt .gitignore
git commit -m "build: add cmake project baseline"
```

### Task 2: Reorganize Source, Header, and Asset Layout

**Files:**
- Create: `D:\Projects\GPA defender\include\gpa_defender\`
- Create: `D:\Projects\GPA defender\src\`
- Create: `D:\Projects\GPA defender\tests\`
- Create: `D:\Projects\GPA defender\docs\`
- Create: `D:\Projects\GPA defender\assets\`
- Modify: include directives in moved `.cpp` and `.h` files

- [ ] **Step 1: Create the target directory tree**

Run:
```powershell
New-Item -ItemType Directory -Force include\gpa_defender,src,tests,docs,assets,scripts
```
Expected: PASS with the directories created or confirmed

- [ ] **Step 2: Move headers, implementation files, docs, and assets into their final locations**

Run:
```powershell
Move-Item -LiteralPath Block.h,DefenseTower.h,Enemy.h,Enemy_fixed.h,GameEngine.h,GameUI.h,Indicator.h,Obstacle.h,PlayerStats.h,Questionnaire.h,Vector2D.h,WaveManager.h -Destination include\gpa_defender
Move-Item -LiteralPath Block.cpp,DefenseTower.cpp,Enemy.cpp,GameEngine.cpp,GameUI.cpp,Obstacle.cpp,PlayerStats.cpp,Questionnaire.cpp,WaveManager.cpp -Destination src
Move-Item -LiteralPath BackendLoopSmokeTest.cpp -Destination tests
Move-Item -LiteralPath README_BACKEND.md,'ASTI模块说明.md','游戏内容设计.md','防御模块说明.md' -Destination docs
Move-Item -LiteralPath '塔防游戏 Proposal.pdf' -Destination assets
```
Expected: PASS with files relocated out of the repository root

- [ ] **Step 3: Normalize includes to the project include root**

```cpp
#include "gpa_defender/GameEngine.h"
#include "gpa_defender/Block.h"
#include "gpa_defender/Questionnaire.h"
```

Use the same `gpa_defender/...` pattern for moved headers referenced by `.cpp` files and test files.

- [ ] **Step 4: Verify the repository root is now mostly entrypoint-only**

Run: `Get-ChildItem -Name`
Expected: output centered on `.vscode`, `assets`, `docs`, `include`, `scripts`, `src`, `tests`, `CMakeLists.txt`, `README.md`, `.gitignore`

- [ ] **Step 5: Commit the repository layout migration**

```bash
git add include src tests docs assets
git commit -m "refactor: reorganize repository layout"
```

### Task 3: Split Runtime Entry Points and Register Smoke Tests

**Files:**
- Create: `D:\Projects\GPA defender\src\full_flow_scenario.h`
- Create: `D:\Projects\GPA defender\src\full_flow_scenario.cpp`
- Create: `D:\Projects\GPA defender\src\main.cpp`
- Create: `D:\Projects\GPA defender\tests\backend_smoke_main.cpp`
- Create: `D:\Projects\GPA defender\tests\full_flow_smoke_main.cpp`
- Modify: `D:\Projects\GPA defender\tests\BackendLoopSmokeTest.cpp`
- Modify: `D:\Projects\GPA defender\CMakeLists.txt`

- [ ] **Step 1: Make the future smoke-test targets fail before their entrypoints exist**

Add the target declarations to `CMakeLists.txt` before the new files are created:

```cmake
add_library(gpa_defender_core STATIC)
add_executable(gpa_defender_app src/main.cpp)
add_executable(backend_smoke tests/backend_smoke_main.cpp tests/BackendLoopSmokeTest.cpp)
add_executable(full_flow_smoke tests/full_flow_smoke_main.cpp)
```

Run: `cmake --build build`
Expected: FAIL with missing source-file errors for the new entrypoint files

- [ ] **Step 2: Extract the full-flow scenario into a reusable function**

```cpp
// src/full_flow_scenario.h
#pragma once

int runFullFlowScenario();
```

```cpp
// src/main.cpp
#include "full_flow_scenario.h"

int main() {
    return runFullFlowScenario();
}
```

```cpp
// tests/full_flow_smoke_main.cpp
#include "full_flow_scenario.h"

int main() {
    return runFullFlowScenario();
}
```

Move the current `main.cpp` body into `runFullFlowScenario()` inside `src/full_flow_scenario.cpp`.

- [ ] **Step 3: Add a real main function for the backend smoke test**

```cpp
// tests/backend_smoke_main.cpp
#include <iostream>

bool runBackendLoopSmokeTest();

int main() {
    if (!runBackendLoopSmokeTest()) {
        std::cout << "[Test] Backend loop smoke test failed.\n";
        return 1;
    }

    std::cout << "[Test] Backend loop smoke test passed.\n";
    return 0;
}
```

- [ ] **Step 4: Finalize the CMake target graph and register CTest entries**

```cmake
add_library(gpa_defender_core STATIC
    src/Block.cpp
    src/DefenseTower.cpp
    src/Enemy.cpp
    src/GameEngine.cpp
    src/GameUI.cpp
    src/Obstacle.cpp
    src/PlayerStats.cpp
    src/Questionnaire.cpp
    src/WaveManager.cpp
    src/full_flow_scenario.cpp
)

target_include_directories(gpa_defender_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(gpa_defender_app src/main.cpp)
target_link_libraries(gpa_defender_app PRIVATE gpa_defender_core)

add_executable(backend_smoke
    tests/backend_smoke_main.cpp
    tests/BackendLoopSmokeTest.cpp
)
target_link_libraries(backend_smoke PRIVATE gpa_defender_core)

add_executable(full_flow_smoke tests/full_flow_smoke_main.cpp)
target_link_libraries(full_flow_smoke PRIVATE gpa_defender_core)

add_test(NAME backend_smoke COMMAND backend_smoke)
add_test(NAME full_flow_smoke COMMAND full_flow_smoke)
```

- [ ] **Step 5: Build and run the smoke tests through the new flow**

Run:
```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
Expected: PASS with both `backend_smoke` and `full_flow_smoke` reported as passing

- [ ] **Step 6: Commit the executable and test split**

```bash
git add CMakeLists.txt src tests
git commit -m "build: wire app and smoke tests through cmake"
```

### Task 4: Align Scripts, VS Code, and Project Documentation

**Files:**
- Create: `D:\Projects\GPA defender\README.md`
- Modify: `D:\Projects\GPA defender\scripts\build_and_test.bat`
- Modify: `D:\Projects\GPA defender\scripts\build_and_test.sh`
- Modify: `D:\Projects\GPA defender\.vscode\tasks.json`
- Modify: `D:\Projects\GPA defender\.vscode\launch.json`
- Modify: `D:\Projects\GPA defender\docs\README_BACKEND.md` if path notes are stale

- [ ] **Step 1: Replace hand-written compile scripts with CMake wrappers**

```bat
@echo off
cmake -S "%~dp0.." -B "%~dp0..\\build" || exit /b 1
cmake --build "%~dp0..\\build" || exit /b 1
ctest --test-dir "%~dp0..\\build" --output-on-failure || exit /b 1
```

```bash
#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build"
cmake --build "$ROOT_DIR/build"
ctest --test-dir "$ROOT_DIR/build" --output-on-failure
```

- [ ] **Step 2: Update VS Code tasks and launch configuration to the build tree**

```json
{
  "label": "CMake: build",
  "type": "shell",
  "command": "cmake",
  "args": ["--build", "${workspaceFolder}/build"],
  "group": { "kind": "build", "isDefault": true }
}
```

```json
{
  "name": "C/C++: Run gpa_defender_app",
  "type": "cppdbg",
  "request": "launch",
  "program": "${workspaceFolder}/build/bin/gpa_defender_app",
  "cwd": "${workspaceFolder}",
  "preLaunchTask": "CMake: build"
}
```

- [ ] **Step 3: Write the new root README around the reorganized structure**

```markdown
# GPA Defender

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Test

```bash
ctest --test-dir build --output-on-failure
```
```

Expand that skeleton to include project purpose, prerequisites, directory layout, and VS Code notes.

- [ ] **Step 4: Run the wrapper script and one editor-equivalent build to verify alignment**

Run:
```powershell
.\scripts\build_and_test.bat
cmake --build build --config Debug
```
Expected: PASS without depending on root-level `.exe` artifacts

- [ ] **Step 5: Commit tooling and documentation alignment**

```bash
git add README.md .vscode scripts docs
git commit -m "docs: align tooling and developer docs"
```

### Task 5: Final Verification and Repository Cleanup

**Files:**
- Modify: repository root tracked file set
- Test: full end-to-end verification commands

- [ ] **Step 1: Remove stale root-level binary artifacts from version control and workspace**

Run:
```powershell
Remove-Item -LiteralPath backend_smoke.exe,full_flow_smoke.exe,main.exe,project_main_check.exe -ErrorAction SilentlyContinue
```
Expected: PASS with generated binaries removed from the root

- [ ] **Step 2: Verify the root layout, build, and tests from a clean state**

Run:
```powershell
if (Test-Path build) { Remove-Item -Recurse -Force build }
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
Get-ChildItem -Name
```
Expected: PASS with both smoke tests green and the root layout matching the spec

- [ ] **Step 3: Review the final diff before the closing commit**

Run:
```bash
git status --short
git diff --stat
```
Expected: only planned initialization changes

- [ ] **Step 4: Commit the completed initialization**

```bash
git add -A
git commit -m "feat: initialize project structure"
```
