#!/bin/bash
set -e

echo "[DEBUG] Building tests..."
echo "[DEBUG] $(pwd)"

mkdir -p build
cd build

cmake .. -DBUILD_TESTS=ON -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
make vnpu_test

echo "[DEBUG] Running tests..."
./test/vnpu_test

bash ../coverage.sh

echo "[DEBUG] Done."
