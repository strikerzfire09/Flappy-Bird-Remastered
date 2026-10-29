#!/usr/bin/env bash
# Build Flappy Bird on Linux/macOS (Unix).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_ROOT="${SCRIPT_DIR}/build"
if [ "$(uname -s)" = "Darwin" ]; then
    PLATFORM="macos"
else
    PLATFORM="linux"
fi
BUILD_DIR="${BUILD_ROOT}/${PLATFORM}"
EXE="${BUILD_DIR}/bin/flappybird"

# A successful previous build is replaced rather than reused. This avoids
# stale CMake caches, old object files, and an outdated executable being run
# after the sources have changed.
if [ -f "${EXE}" ]; then
    echo "==> Existing build found; removing ${BUILD_DIR}"
    rm -rf -- "${BUILD_DIR}"
elif [ -f "${BUILD_ROOT}/bin/flappybird" ]; then
    # Remove the old, pre-platform-folder build layout.
    echo "==> Legacy build found; removing ${BUILD_ROOT}"
    rm -rf -- "${BUILD_ROOT}"
fi

echo "==> Configuring Flappy Bird (Unix)"
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release "${@}"

echo "==> Building"
cmake --build "${BUILD_DIR}" -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

echo
echo "==> Done. Run:  ${EXE}"
