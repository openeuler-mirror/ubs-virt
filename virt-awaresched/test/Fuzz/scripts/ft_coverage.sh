#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
VA_ROOT=$(cd "$SCRIPT_DIR/../../.." && pwd)
BUILD_DIR=${FT_BUILD_DIR:-$VA_ROOT/build-fuzz-coverage}
DEPS_DIR=${FT_DEPS_DIR:-$BUILD_DIR/_deps}
BOOTSTRAP=$VA_ROOT/test/Fuzz/CMakeLists.txt
RESULT_DIR=${FT_RESULT_DIR:-$BUILD_DIR/results/coverage}

cmake -S "$VA_ROOT" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTS=OFF \
    -DDEPS_DIR="$DEPS_DIR" \
    -DCMAKE_PROJECT_INCLUDE="$BOOTSTRAP" \
    -DFT_COVERAGE=ON \
    -DENABLE_COVERAGE=ON
cmake --build "$BUILD_DIR" --target fuzz_tests -j"${FT_JOBS:-8}"

mkdir -p "$RESULT_DIR"
find "$BUILD_DIR" -type f -name '*.gcda' -delete

export VIRT_CURL_PATH=${VIRT_CURL_PATH:-/usr/lib64}
for target in \
    VirtAwareSched_parser_ut \
    VirtAwareSched_cmd_serialize_ut \
    VirtAwareSched_socket_server_ut \
    VirtAwareSched_socket_client_ut \
    VirtAwareSched_api_ut; do
    "$BUILD_DIR/bin/$target" --gtest_output="xml:$RESULT_DIR/$target.xml"
done

VAS_CI=1 "$BUILD_DIR/bin/VirtAwareSched_security_ut" \
    --gtest_output="xml:$RESULT_DIR/VirtAwareSched_security_ut.xml"

for target in ft_va_cmd_deser ft_va_cli_parse; do
    "$BUILD_DIR/bin/$target" \
        --runs="${FT_SMOKE_RUNS:-10000}" \
        --time="${FT_SMOKE_SECONDS:-300}" \
        --hang-timeout="${FT_HANG_TIMEOUT_MS:-5000}" \
        --seed="${FT_SEED:-20260924}" \
        --corpus="$BUILD_DIR/corpus/$target" \
        --crash-dir="$BUILD_DIR/crashes/$target" \
        >"$RESULT_DIR/${target}_smoke.log" 2>&1
done

FT_BUILD_DIR="$BUILD_DIR" FT_RESULT_DIR="$RESULT_DIR" "$SCRIPT_DIR/gcov_summary.sh"
