#!/bin/bash
set -e

echo "[DEBUG] Building tests..."
echo "[DEBUG] $(pwd)"

if [ -z "$ASCEND_HOME_PATH" ]; then
    echo "[ERROR] ASCEND_HOME_PATH is not set!"
    exit 1
fi

mkdir -p __build
cd __build

# 在ut用例运行前,把测试所需的test_npu_info.config拷贝到临时目录下面
cp ../test/res/test_npu_info.config .

cmake .. -DBUILD_TESTS=ON -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
make vnpu_test
# mem-swap 模块独立单测(自制断言框架, 各含独立 main), 需单独编译并运行以计入覆盖率
make memory_tracker_test swap_executor_test swap_hook_test

echo "[DEBUG] Running tests..."
./test/vnpu_test --gtest_output=xml:test_detail.xml
./test/memory_tracker_test
./test/swap_executor_test
./test/swap_hook_test

bash ../coverage.sh

echo "[DEBUG] Done."
