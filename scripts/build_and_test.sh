#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
CMAKE_CMD="${CMAKE_EXE:-cmake}"

if ! command -v "$CMAKE_CMD" >/dev/null 2>&1; then
    echo "ERROR: cmake not found."
    echo "Hint: install CMake or set CMAKE_EXE to the cmake executable path."
    exit 1
fi

"$CMAKE_CMD" -S "$ROOT_DIR" -B "$BUILD_DIR"
"$CMAKE_CMD" --build "$BUILD_DIR"
ctest --test-dir "$BUILD_DIR" --output-on-failure
