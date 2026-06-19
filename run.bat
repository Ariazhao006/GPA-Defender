@echo off
chcp 65001 >nul
cd /d "%~dp0"
if not exist "build\bin\gpa_defender_gui.exe" (
    echo 正在编译项目...
    if not exist "build" mkdir build
    "C:\msys64\ucrt64\bin\cmake.exe" -G "MinGW Makefiles" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_C_COMPILER="C:/msys64/ucrt64/bin/clang.exe" -DCMAKE_CXX_COMPILER="C:/msys64/ucrt64/bin/clang++.exe" -DCMAKE_MAKE_PROGRAM="C:/msys64/mingw64/bin/mingw32-make.exe" -S . -B build
    if errorlevel 1 (
        echo CMake 配置失败！
        pause
        exit /b 1
    )
    "C:\msys64\mingw64\bin\mingw32-make.exe" -C build -j%NUMBER_OF_PROCESSORS%
    if errorlevel 1 (
        echo 编译失败！
        pause
        exit /b 1
    )
)
echo 启动 GPA Defender...
start "" "build\bin\gpa_defender_gui.exe"
