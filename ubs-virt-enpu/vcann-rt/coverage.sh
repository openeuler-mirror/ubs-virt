#!/bin/bash
set -e

if ! command -v lcov > /dev/null 2>&1; then
    echo "[ERROR] Command lcov not found."
    exit 1
fi

echo "[DEBUG] Generating coverage report..."
echo "[DEBUG] $(pwd)"

COVERAGE_THRESHOLD=80.0

IGNORED_ERRORS="mismatch,unused,unused"

lcov --capture --directory . \
    --ignore-errors "$IGNORED_ERRORS" \
    --output-file coverage_base.info
lcov --remove coverage_base.info \
    '/usr/*' '*/test/*' '*/build/*' \
    '*.h' '*.hpp' '*.hh' '*.hxx' \
    --ignore-errors "$IGNORED_ERRORS" \
    --output-file coverage_filtered.info

echo "[DEBUG] Generating html report..."
if command -v genhtml > /dev/null 2>&1; then
    genhtml coverage_filtered.info --output-directory coverage_report
    echo "[DEBUG] Coverage html report generated to $(pwd)/coverage_report."
else
    echo "[ERROR] Command genhtml not found."
fi

LINE_COVERAGE=$(lcov --summary coverage_filtered.info \
    --ignore-errors "$IGNORED_ERRORS" \
    2>/dev/null \
    | grep "lines" | sed -E "s#.*: *([0-9]+\.?[0-9]*)% .*#\1#" \
    || echo "0.0")

if [ -n "$LINE_COVERAGE" ]; then
    if (( $(echo "$LINE_COVERAGE >= $COVERAGE_THRESHOLD" | bc -l) )); then
        echo "[INFO] Coverage check PASSED: $LINE_COVERAGE% >= $COVERAGE_THRESHOLD%."
    else
        echo "[ERROR] Coverage check FAILED: $LINE_COVERAGE% < $COVERAGE_THRESHOLD%."
        exit 1
    fi
else
    echo "[DEBUG] Error: failed to get line coverage."
    exit 1
fi

echo "[DEBUG] Done."
