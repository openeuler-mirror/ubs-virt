#!/bin/bash
set -e
PROJECT_NAME=vcann-runtime
VERSION=1.0

CURRENT_PATH=$(cd "$(dirname "$0")"; pwd)

if [ -z "$ASCEND_HOME_PATH" ] || [ -z "$ASCEND_TOOLKIT_HOME" ]; then
    echo "[ERROR] ASCEND_HOME_PATH OR ASCEND_TOOLKIT_HOME is not set!"
    exit 1
fi

# ========== CANN Verison =========
CANN_VERSION=${1:-"8.3.0"}

IF_VALID_FORMAT='^[0-9]+(\.[0-9]+)*$'

if [[ ! "$CANN_VERSION" =~ $IF_VALID_FORMAT ]]; then
    echo "Error: Invalid version format: '$CANN_VERSION'"
    echo "Only numeric versions (e.g., 8.5 or 8.5.0) are supported."
    exit 1
fi

BASE_VERSION="8.5.0"

IS_NEW_VER=$(printf "%s\n%s" "$BASE_VERSION" "$CANN_VERSION" | sort -V | head -n1)
if [ "$IS_NEW_VER" = "$BASE_VERSION" ]; then
    #  8.5.0+
    BUILD_WITH_NEW=1
    echo "$CANN_VERSION is greater than or equal to $BASE_VERSION set BUILD_WITH_NEW to 1"
else
    #  8.4.x
    BUILD_WITH_NEW=0
    echo "$CANN_VERSION is less than $BASE_VERSION set BUILD_WITH_NEW to 0"
fi

BUILD_PATH="$CURRENT_PATH/build"
mkdir -p "$BUILD_PATH"
cd "$BUILD_PATH"

if [ "$BUILD_WITH_NEW" -eq 1 ]; then
    CMAKE_CMD="cmake .. -DENABLE_NEW_BUILD=ON"
else
    CMAKE_CMD="cmake .. -DENABLE_NEW_BUILD=OFF"
fi

if ! eval "$CMAKE_CMD"; then
    echo "[ERROR] make_build:cmake failed.!"
    exit 1
fi

if ! make -j $(nproc); then
    echo "[ERROR] make_build:make failed."
    exit 1
fi
