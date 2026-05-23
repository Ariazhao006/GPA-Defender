@echo off
setlocal

set "ROOT_DIR=%~dp0.."
set "BUILD_DIR=%ROOT_DIR%\build"

if defined CMAKE_EXE (
    set "CMAKE_CMD=%CMAKE_EXE%"
) else (
    where cmake >nul 2>nul
    if errorlevel 1 (
        echo ERROR: cmake not found in PATH.
        echo Hint: install CMake or set the CMAKE_EXE environment variable.
        exit /b 1
    )
    set "CMAKE_CMD=cmake"
)

"%CMAKE_CMD%" -S "%ROOT_DIR%" -B "%BUILD_DIR%" || exit /b 1
"%CMAKE_CMD%" --build "%BUILD_DIR%" || exit /b 1
ctest --test-dir "%BUILD_DIR%" --output-on-failure || exit /b 1
