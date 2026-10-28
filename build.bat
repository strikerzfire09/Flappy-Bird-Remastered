@echo off
setlocal enabledelayedexpansion

echo ==> Cleaning old build directory...
if exist build (
    rmdir /s /q build
)

echo ==> Configuring Flappy Bird for Windows (MinGW)
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

if %errorlevel% neq 0 (
    echo Configuration failed.
    exit /b %errorlevel%
)

echo ==> Building
cmake --build build --config Release

if %errorlevel% neq 0 (
    echo Build failed.
    exit /b %errorlevel%
)

echo.
echo ==> Done. Run:  build\flappybird.exe