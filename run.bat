@echo off
setlocal

cd /d "%~dp0"

set "CMAKE_EXE=C:\msys64\ucrt64\bin\cmake.exe"
set "MAKE_EXE=C:\msys64\mingw64\bin\mingw32-make.exe"
set "CC_EXE=C:/msys64/ucrt64/bin/clang.exe"
set "CXX_EXE=C:/msys64/ucrt64/bin/clang++.exe"
set "APP_EXE=build\bin\gpa_defender_gui.exe"

echo Checking and building GPA Defender...

if not exist "%CMAKE_EXE%" (
    echo Missing CMake: %CMAKE_EXE%
    pause
    exit /b 1
)

if not exist "%MAKE_EXE%" (
    echo Missing mingw32-make: %MAKE_EXE%
    pause
    exit /b 1
)

if not exist "build" mkdir build

"%CMAKE_EXE%" -G "MinGW Makefiles" "-DCMAKE_POLICY_VERSION_MINIMUM=3.5" "-DCMAKE_C_COMPILER=%CC_EXE%" "-DCMAKE_CXX_COMPILER=%CXX_EXE%" "-DCMAKE_MAKE_PROGRAM=%MAKE_EXE%" -S . -B build
if errorlevel 1 (
    echo CMake configure failed.
    pause
    exit /b 1
)

"%MAKE_EXE%" -C build gpa_defender_gui -j%NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo Build failed. If the game window is open, close it and run run.bat again.
    pause
    exit /b 1
)

echo Starting GPA Defender...
start "" "%APP_EXE%"
