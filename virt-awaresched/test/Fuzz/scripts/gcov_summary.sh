#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
VA_ROOT=$(cd "$SCRIPT_DIR/../../.." && pwd)
BUILD_DIR=${FT_BUILD_DIR:-$VA_ROOT/build-fuzz-coverage}
RESULT_DIR=${FT_RESULT_DIR:-$BUILD_DIR/results/coverage}
THRESHOLD=${FT_LINE_THRESHOLD:-85}
RAW_INFO=$RESULT_DIR/coverage_raw.info
FILTERED_INFO=$RESULT_DIR/coverage_filtered.info

mkdir -p "$RESULT_DIR"

if command -v lcov >/dev/null 2>&1; then
    lcov --capture --directory "$BUILD_DIR" --output-file "$RAW_INFO"
    lcov --extract "$RAW_INFO" \
        "$VA_ROOT/src/cli/parser/*" \
        "$VA_ROOT/src/cli/opt_serialize/*" \
        "$VA_ROOT/src/cli/socket/*" \
        "$VA_ROOT/src/vasd/api/*" \
        --output-file "$FILTERED_INFO"

    check_module()
    {
        local name=$1
        local pattern=$2
        local info=$RESULT_DIR/${name//\//_}.info
        local summary
        local rate

        lcov --extract "$FILTERED_INFO" "$VA_ROOT/src/$pattern/*" --output-file "$info" >/dev/null
        summary=$(lcov --summary "$info" 2>&1)
        echo "$name"
        echo "$summary"
        # lcov prints the rate after a colon, for example
        # "  lines......: 94.0% (...)". Split on that colon explicitly;
        # whitespace-separated field $2 is not the lcov value format.
        rate=$(printf '%s\n' "$summary" | awk -F: '/lines.*:/ {
            value = $2
            sub(/^[[:space:]]*/, "", value)
            sub(/%.*/, "", value)
            print value
            exit
        }')
        if [[ -z "$rate" ]]; then
            echo "failed to parse lcov line coverage for $name" >&2
            exit 1
        fi
        awk -v rate="$rate" -v threshold="$THRESHOLD" 'BEGIN { exit !(rate + 0 >= threshold + 0) }'
    }

    check_module cli/parser cli/parser
    check_module cli/opt_serialize cli/opt_serialize
    check_module cli/socket cli/socket
    check_module vasd/api vasd/api

    echo "combined"
    lcov --summary "$FILTERED_INFO"
elif python3 -m gcovr --version >/dev/null 2>&1; then
    check_module()
    {
        local name=$1
        local pattern=$2
        local stem=${name//\//_}

        echo "$name"
        python3 -m gcovr \
            --root "$VA_ROOT" \
            --object-directory "$BUILD_DIR" \
            --filter "$VA_ROOT/src/$pattern/.*" \
            --txt "$RESULT_DIR/$stem.txt" \
            --json-summary "$RESULT_DIR/$stem.json" \
            --print-summary \
            --fail-under-line "$THRESHOLD"
    }

    check_module cli/parser cli/parser
    check_module cli/opt_serialize cli/opt_serialize
    check_module cli/socket cli/socket
    check_module vasd/api vasd/api

    echo "combined"
    python3 -m gcovr \
        --root "$VA_ROOT" \
        --object-directory "$BUILD_DIR" \
        --filter "$VA_ROOT/src/(cli/parser|cli/opt_serialize|cli/socket|vasd/api)/.*" \
        --txt "$RESULT_DIR/combined.txt" \
        --json-summary "$RESULT_DIR/combined.json" \
        --print-summary
else
    echo "lcov or gcovr is required to summarize coverage" >&2
    exit 2
fi
