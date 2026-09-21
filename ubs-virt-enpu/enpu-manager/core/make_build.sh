#!/bin/bash
set -e

PROJECT_NAME=enpu-manager

CURRENT_PATH=$(cd "$(dirname "$0")"; pwd)
VERSION=$(cat "$CURRENT_PATH/../VERSION" 2>/dev/null || echo "1.0.0")

if [ -z "$ASCEND_HOME_PATH" ]; then
    echo "[ERROR] ASCEND_HOME_PATH is not set!"
    exit 1
fi

if [ -z "$ENPU_ASCEND_DRIVER_PATH" ]; then
    export ENPU_ASCEND_DRIVER_PATH="/usr/local/Ascend"
    echo "[WARNING] ENPU_ASCEND_DRIVER_PATH is not set, using default: $ENPU_ASCEND_DRIVER_PATH"
fi

BUILD_PATH="$CURRENT_PATH/build"
OUTPUT_PATH="$CURRENT_PATH/build/output"

mkdir -p "$BUILD_PATH"
mkdir -p "$OUTPUT_PATH"

cd "$BUILD_PATH"

echo "[INFO] Running cmake..."
if ! cmake ..; then
    echo "[ERROR] CMake configuration failed"
    exit 1
fi

echo "[INFO] Building project..."
if ! make -j$(nproc 2>/dev/null || echo 4); then
    echo "[ERROR] Build failed"
    exit 1
fi

echo "[INFO] Copying output files..."
cp -f libenpu_manager.so* "$OUTPUT_PATH/" 2>/dev/null || true

echo "[INFO] Build complete: $OUTPUT_PATH"
echo "[INFO] Output files:"
ls -la "$OUTPUT_PATH/" 2>/dev/null || echo "  (empty)"