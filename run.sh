#!/usr/bin/env bash
# 在 Git Bash / MSYS2 环境中运行 GPA Defender

set -e

cd "$(dirname "$0")"

EXE="build/bin/gpa_defender_gui.exe"

if [[ ! -f "$EXE" ]]; then
    echo "正在编译项目..."
    if [[ ! -d "build" ]]; then
        mkdir build
    fi

    cmake -S . -B build \
        -G "MinGW Makefiles" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DCMAKE_C_COMPILER="C:/msys64/ucrt64/bin/clang.exe" \
        -DCMAKE_CXX_COMPILER="C:/msys64/ucrt64/bin/clang++.exe" \
        -DCMAKE_MAKE_PROGRAM="C:/msys64/mingw64/bin/mingw32-make.exe"

    mingw32-make -C build -j"$(nproc)"
fi

echo "启动 GPA Defender..."
"$EXE" &
