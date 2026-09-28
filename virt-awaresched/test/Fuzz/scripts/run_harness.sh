#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
VA_ROOT=$(cd "$SCRIPT_DIR/../../.." && pwd)
BUILD_DIR=${FT_BUILD_DIR:-$VA_ROOT/build-fuzz}
DEPS_DIR=${FT_DEPS_DIR:-$BUILD_DIR/_deps}
BOOTSTRAP=$VA_ROOT/test/Fuzz/CMakeLists.txt
RUN_ID=${FT_RUN_ID:-$(date -u '+%Y%m%dT%H%M%SZ')}
RESULT_DIR=${FT_RESULT_DIR:-$BUILD_DIR/results/fuzz/$RUN_ID}
RUNS=${FT_RUNS:-30000000}
SECONDS_LIMIT=${FT_SECONDS:-10800}
HANG_TIMEOUT_MS=${FT_HANG_TIMEOUT_MS:-5000}
BASE_SEED=${FT_SEED:-20260922}

cmake -S "$VA_ROOT" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="${FT_BUILD_TYPE:-Debug}" \
    -DBUILD_TESTS=OFF \
    -DDEPS_DIR="$DEPS_DIR" \
    -DCMAKE_PROJECT_INCLUDE="$BOOTSTRAP" \
    -DFT_SANITIZE="${FT_SANITIZE:-OFF}"
cmake --build "$BUILD_DIR" --target ft_va_cmd_deser ft_va_cli_parse -j"${FT_JOBS:-8}"

mkdir -p "$RESULT_DIR"
{
    echo "run_id=$RUN_ID"
    echo "build_dir=$BUILD_DIR"
    echo "runs=$RUNS"
    echo "seconds=$SECONDS_LIMIT"
    echo "hang_timeout_ms=$HANG_TIMEOUT_MS"
    echo "base_seed=$BASE_SEED"
    echo "git_commit=$(git -C "$VA_ROOT" rev-parse HEAD 2>/dev/null || echo unknown)"
    echo "git_dirty=$(git -C "$VA_ROOT" status --porcelain --untracked-files=no 2>/dev/null | wc -l)"
} >"$RESULT_DIR/run.meta"

monitor_pid()
{
    local pid=$1
    local log=$2
    while kill -0 "$pid" 2>/dev/null; do
        local rss
        local fds
        rss=$(awk '/VmRSS/{print $2}' "/proc/$pid/status" 2>/dev/null || true)
        fds=$(find "/proc/$pid/fd" -mindepth 1 -maxdepth 1 2>/dev/null | wc -l)
        echo "$(date '+%F %T') rss_kb=${rss:-NA} fd=${fds:-NA}" >>"$log"
        sleep 10
    done
}

overall_rc=0
harness_index=0
for target in ft_va_cmd_deser ft_va_cli_parse; do
    corpus_dir=$BUILD_DIR/corpus/$target
    crash_dir=$BUILD_DIR/crashes/$target
    rm -rf -- "$corpus_dir" "$crash_dir"
    mkdir -p "$corpus_dir" "$crash_dir"
    : >"$RESULT_DIR/${target}_monitor.log"

    set +e
    "$BUILD_DIR/bin/$target" \
        --runs="$RUNS" \
        --time="$SECONDS_LIMIT" \
        --hang-timeout="$HANG_TIMEOUT_MS" \
        --seed="$((BASE_SEED + harness_index))" \
        --corpus="$corpus_dir" \
        --crash-dir="$crash_dir" \
        --quiet >"$RESULT_DIR/${target}_final.log" 2>&1 &
    harness_pid=$!
    monitor_pid "$harness_pid" "$RESULT_DIR/${target}_monitor.log" &
    monitor_process=$!
    wait "$harness_pid"
    rc=$?
    kill "$monitor_process" 2>/dev/null || true
    wait "$monitor_process" 2>/dev/null || true
    set -e

    peak_rss=$(sed -n 's/.*rss_kb=\([0-9][0-9]*\).*/\1/p' "$RESULT_DIR/${target}_monitor.log" | sort -n | tail -1)
    peak_fd=$(sed -n 's/.*fd=\([0-9][0-9]*\).*/\1/p' "$RESULT_DIR/${target}_monitor.log" | sort -n | tail -1)
    echo "[ft] peak_rss_kb=${peak_rss:-NA} peak_fd=${peak_fd:-NA}" >>"$RESULT_DIR/${target}_final.log"
    if [[ $rc -ne 0 ]]; then
        overall_rc=$rc
    fi
    harness_index=$((harness_index + 1))
done

echo "RESULT_DIR=$RESULT_DIR"
exit "$overall_rc"
