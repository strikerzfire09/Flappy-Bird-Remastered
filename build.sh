#!/usr/bin/env bash

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="build/linux"
BUILD_TYPE="Release"

echo
echo "======================================"
echo "       Flappy Bird Linux Build"
echo "======================================"
echo

# Dependencies
for cmd in cmake ninja python3 git; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "ERROR: $cmd is not installed."
        exit 1
    fi
done

# Ask for Raylib directory
DEFAULT_RAYLIB_DIR="third_party/raylib"

echo "Raylib source directory."
echo "Press ENTER to use the default: $DEFAULT_RAYLIB_DIR"
read -r -p "Raylib path: " RAYLIB_DIR

RAYLIB_DIR="${RAYLIB_DIR:-$DEFAULT_RAYLIB_DIR}"

# Resolve relative paths
if [[ "$RAYLIB_DIR" != /* ]]; then
    RAYLIB_DIR="$SCRIPT_DIR/$RAYLIB_DIR"
fi

# Download Raylib if missing
if [[ ! -d "$RAYLIB_DIR" ]]; then
    echo
    echo "Raylib not found. Downloading..."

    mkdir -p "$(dirname "$RAYLIB_DIR")"

    git clone --depth 1 \
        https://github.com/raysan5/raylib.git \
        "$RAYLIB_DIR"
fi

# Validate Raylib
if [[ ! -f "$RAYLIB_DIR/CMakeLists.txt" ]]; then
    echo "ERROR: Invalid Raylib directory: $RAYLIB_DIR"
    exit 1
fi

# Compiler
CC="${CC:-gcc}"
CXX="${CXX:-g++}"

echo
echo "Raylib: $RAYLIB_DIR"
echo "Compiler: $CC / $CXX"
echo

echo "[1/2] Configuring..."

cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_C_COMPILER="$CC" \
    -DCMAKE_CXX_COMPILER="$CXX" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DFLAPPYBIRD_RAYLIB_SOURCE_DIR="$RAYLIB_DIR"

echo
echo "[2/2] Building..."

cmake --build "$BUILD_DIR" --parallel

echo
echo "======================================"
echo "          BUILD SUCCESSFUL"
echo "======================================"
echo
echo "Executable: $BUILD_DIR/bin/flappybird"