@echo off
setlocal EnableExtensions

title Flappy Bird - Windows Build

cd /d "%~dp0"
if errorlevel 1 exit /b 1

set "BUILD_DIR=build/windows"
set "BUILD_TYPE=Release"

set "LLVM_MINGW=C:\llvm-mingw-20261020-ucrt-x86_64"

echo.
echo ======================================
echo       Flappy Bird Windows Build
echo ======================================
echo.

REM Check dependencies
where cmake >nul 2>&1 || goto :missing_cmake
where ninja >nul 2>&1 || goto :missing_ninja
where python >nul 2>&1 || goto :missing_python
where git >nul 2>&1 || goto :missing_git

REM Check LLVM-MinGW
if not exist "%LLVM_MINGW%\bin\clang.exe" (
    echo ERROR: LLVM-MinGW clang.exe not found.
    goto :error
)

if not exist "%LLVM_MINGW%\bin\clang++.exe" (
    echo ERROR: LLVM-MinGW clang++.exe not found.
    goto :error
)

set "PATH=%LLVM_MINGW%\bin;%PATH%"

REM Ask for Raylib directory
echo Raylib source directory.
echo Press ENTER to use the default: third_party\raylib
echo.

set "RAYLIB_INPUT="
set /p "RAYLIB_INPUT=Raylib path: "

if not defined RAYLIB_INPUT (
    set "RAYLIB_INPUT=third_party\raylib"
)

set "RAYLIB_DIR=%RAYLIB_INPUT%"

REM Resolve absolute path
for %%I in ("%RAYLIB_DIR%") do set "RAYLIB_DIR=%%~fI"

REM Download Raylib if missing
if not exist "%RAYLIB_DIR%" (
    echo.
    echo Raylib not found. Downloading...

    for %%I in ("%RAYLIB_DIR%\..") do (
        if not exist "%%~fI" mkdir "%%~fI"
    )

    git clone --depth 1 https://github.com/raysan5/raylib.git "%RAYLIB_DIR%"

    if errorlevel 1 goto :error
)

REM Validate Raylib
if not exist "%RAYLIB_DIR%\CMakeLists.txt" (
    echo ERROR: Invalid Raylib directory.
    echo Path: %RAYLIB_DIR%
    goto :error
)

echo.
echo Raylib: %RAYLIB_DIR%
echo.

echo [1/2] Configuring...

cmake -S "%~dp0" -B "%BUILD_DIR%" -G Ninja ^
    -DCMAKE_C_COMPILER=clang ^
    -DCMAKE_CXX_COMPILER=clang++ ^
    -DCMAKE_C_COMPILER_TARGET=x86_64-w64-windows-gnu ^
    -DCMAKE_CXX_COMPILER_TARGET=x86_64-w64-windows-gnu ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DFLAPPYBIRD_RAYLIB_SOURCE_DIR="%RAYLIB_DIR%"

if errorlevel 1 goto :error

echo.
echo [2/2] Building...

cmake --build "%BUILD_DIR%" --parallel

if errorlevel 1 goto :error

echo.
echo ======================================
echo          BUILD SUCCESSFUL
echo ======================================
echo.
echo Executable: %BUILD_DIR%\bin\flappybird.exe
echo.

pause
exit /b 0

:missing_cmake
echo ERROR: CMake not found.
goto :error

:missing_ninja
echo ERROR: Ninja not found.
goto :error

:missing_python
echo ERROR: Python not found.
goto :error

:missing_git
echo ERROR: Git not found.
goto :error

:error
echo.
echo ======================================
echo             BUILD FAILED
echo ======================================
echo.
pause
exit /b 1