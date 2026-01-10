#!/bin/bash
set -euo pipefail

ROOT_DIR=$(dirname "$(realpath "$0")")
BUILD_DIR="$ROOT_DIR/buildUT"
COVERAGE=true

log() {
  echo "[INFO] $*"
}
err() {
  echo "[ERROR] $*" >&2
}

rm -rf "$BUILD_DIR"

if $COVERAGE; then
  log "config with COVERAGE ON"
  cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DBUILD_TESTS=ON -DCOVERAGE=ON
fi

log "Building UT"
cmake --build "$BUILD_DIR" -j"$(nproc)"

log "Running UT"
cd "$BUILD_DIR/test"
ctest --output-on-failure

if $COVERAGE; then
  log "Gen report"
  cmake --build "$BUILD_DIR" --target coverage
  coverageInfo="$BUILD_DIR/src/coverage.info"
  coverageXml="$BUILD_DIR/src/coverage/test_detail.xml"
  coverageHtml="$BUILD_DIR/src/coverage_html"
  REPORT_DIR="$ROOT_DIR/gcover_report"

  rm -rf "$REPORT_DIR"
  mkdir -p "$REPORT_DIR"
  cp "$coverageInfo" "$REPORT_DIR/"
  cp "$coverageXml" "$REPORT_DIR/"
  cp "$coverageHtml" "$REPORT_DIR/"
  log "Coverage and test report files are ready"

fi