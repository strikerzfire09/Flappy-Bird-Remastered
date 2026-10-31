#!/usr/bin/env bash

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="build/macos"
BUILD_TYPE="Release"
RAYLIB_DIR="third_party/raylib"

echo
echo "======================================"
echo "      Flappy Bird macOS Build"
echo "======================================"
echo

# Check dependencies
for cmd in cmake ninja python3 git clang clang++; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "ERROR: $cmd is not installed."
        exit 1
    fi
done

# Download Raylib if missing
if [[ ! -d "$RAYLIB_DIR" ]]; then
    echo "Raylib not found. Downloading..."

    mkdir -p "$(dirname "$RAYLIB_DIR")"

    git clone --depth 1 \
        https://github.com/raysan5/raylib.git \
        "$RAYLIB_DIR"
fi

# Validate Raylib
if [[ ! -f "$RAYLIB_DIR/CMakeLists.txt" ]]; then
    echo "ERROR: Invalid Raylib directory."
    exit 1
fi

echo "Raylib: $RAYLIB_DIR"
echo "Compiler: clang / clang++"

echo
echo "[1/2] Configuring..."

cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DFLAPPYBIRD_RAYLIB_SOURCE_DIR="$SCRIPT_DIR/$RAYLIB_DIR"

echo
echo "[2/2] Building..."

cmake --build "$BUILD_DIR" --parallel

echo
echo "======================================"
echo "         BUILD SUCCESSFUL"
echo "======================================"
echo
echo "Executable: $BUILD_DIR/bin/flappybird"