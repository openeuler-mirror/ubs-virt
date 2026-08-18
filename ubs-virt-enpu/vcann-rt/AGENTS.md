# AGENTS.md

## Project Overview

vCANN-RT (virtual CANN Runtime) — NPU resource soft-partitioning solution within `ubs-virt-enpu`.
C11 source code, C++14 test code. CMake project targeting openEuler Linux (ARM64).

Uses Linux `LD_PRELOAD` hook mechanism to intercept Ascend CANN runtime API calls, enforcing
compute (AI Core) and memory (HBM) resource quotas based on configured allocation policies.

## Build Commands

```shell
# Prerequisites: Set environment variables
export ASCEND_HOME_PATH=/usr/local/Ascend/ascend-toolkit/latest
export ENPU_ASCEND_DRIVER_PATH=/usr/local/Ascend  # optional, default

# Release build (produces libvruntime.so + enpu-monitor in build/)
bash make_build.sh

# Clean build directory
rm -rf build
```

## Test Commands

```shell
# Run all UT tests (build + run + coverage report)
bash build_ut.sh

# Build only tests (without running)
cd __build && cmake .. -DBUILD_TESTS=ON -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug && make vnpu_test

# Run specific test suite
./__build/test/vnpu_test --gtest_filter="DeviceTest*"

# Run specific test case
./__build/test/vnpu_test --gtest_filter="MemoryTest.GuardMemory"
```

Coverage threshold: **80% line coverage** (enforced by `coverage.sh`).

## Code Style

- C11 / C++14 standard
- clang-format for formatting (Google style, 4-space indent, 120 column limit)
- clang-tidy for static analysis
- Comments in Chinese or English as appropriate

## Dev Environment Tips

- Default build type is Release; tests auto-switch to Debug for coverage
- `ASCEND_HOME_PATH` must be set to CANN toolkit path
- `ENPU_ASCEND_DRIVER_PATH` defaults to `/usr/local/Ascend`
- `compile_commands.json` is in `build/` (release) or `__build/` (test)
- Point your LSP to the correct build directory for your configuration
- Build artifacts: `libvruntime.so` (hook library) and `enpu-monitor` (monitoring tool)

## Architecture

```
src/
├── ascend/      # LD_PRELOAD hooks (device, event, memory, kernel, task, graph)
├── tools/       # enpu-monitor tool
├── utils/       # config, dcmi_wrapper, hash_map, utils
└── include/     # Headers

test/
├── testcase/    # Unit tests (gtest + mockcpp)
├── stub/        # CANN runtime & DCMI stubs
└── res/         # Test resources

scripts/
└── cmake/       # CMake modules (securec, gtest, mockcpp)
```

## Security Guidelines

**禁止以下行为（红线规则）：**

1. **禁止提交敏感信息**
   - API 密钥、密码、Token、证书私钥
   - 数据库连接字符串含凭证
   - SSH 私钥、GPG 密钥

2. **禁止硬编码凭证**
   - 用户名/密码
   - Access Key/Secret Key
   - 认证 Token

3. **禁止绕过安全检查**
   - 禁用 SSL/TLS 验证
   - 注释或删除安全相关代码
   - 关闭认证/授权机制
   - 绕过内存安全函数（securec.h）

4. **禁止不安全日志**
   - 记录敏感数据（密码、Token、个人信息）
   - 明文记录凭证

**必须遵守：**

- 使用环境变量或配置文件管理凭证（配置文件需 `.gitignore`）
- 敏感操作需代码审查
- 定期轮换密钥和凭证
- 报告安全漏洞不公开披露
- 始终使用 `securec.h` 提供的安全函数
- 动态库和可执行文件启用安全编译选项

## Commit Guidelines

- Run `bash build_ut.sh` before committing
- Ensure coverage >= 80%
- Ensure clang-format and clang-tidy checks pass
- Add or update unit tests for code changes (stub new APIs in `test/stub/` as needed)
