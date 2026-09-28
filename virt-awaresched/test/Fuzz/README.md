# virt-awaresched 安全测试设计、使用手册与结果报告

报告日期：2026-09-29
测试记录日期：2026-09-22 至 2026-09-29

## 1. 报告范围与结论

本文合并原安全测试设计、测试计划、使用手册和 openEuler ARM64 最终结果，是本次提交中唯一的 Markdown 文档。测试代码、测试 CMake 入口和运行脚本全部位于 `test/Fuzz/`；现有生产源码、生产目录、上层 `CMakeLists.txt` 及既有测试目录保持上游原样。逐次运行日志、GoogleTest XML、覆盖率原始文件、sanitizer 日志及 manifest 未随本 PR 提交，因此历史测试结果仍不是可由当前提交独立核验的完整证据包。

最终 ARM64 复验报告：19 个安全压力接口均完成每接口 30,000,000 次调用，GoogleTest XML 记录 `tests=19`、`failures=0`、`errors=0`；最长单项约 37.7 分钟，未达到 3 小时上限。普通 `BUILD_TESTS=ON` 上游目标收集原版 184 项并全部通过；Fuzz 独立目标复用这些上游测试并额外执行 7 个边界用例，单元回归与压力 smoke 共 91/91 通过。两个 DTFuzz harness 均完成 30,000,000 次执行，0 crash、0 hang。四个目标模块的合计行覆盖率为 91.9%，且每个模块均达到 85% 门槛。详见第 4、5 节。

**重要范围说明：本 PR 不包含生产源码或生产源码注释的改动。** PR 当前保持单一提交；复验时以 PR 页面显示的最新 head SHA 为唯一源码依据，不在报告中嵌入会因后续整理而失效的自引用 SHA。目标机使用的复制快照没有 `.git` 元数据，因此运行元数据仍显示 `git_commit=unknown`；目标机构建完成后，PR 最终提交仅把 22 个 Fuzz 测试文件头的版权年份从 2025 调整为 2026，没有改变任何可执行测试内容。原始日志、XML 与哈希保存在目标机仓库外证据目录，不随 PR 提交。

### 1.1 目录结构

```text
test/Fuzz/
├── CMakeLists.txt
├── README.md
├── driver/
│   ├── CMakeLists.txt
│   ├── ft_driver.cc
│   └── ft_driver.h
├── scripts/
│   ├── ft_coverage.sh
│   ├── gcov_summary.sh
│   └── run_harness.sh
└── virt_awaresched/
    ├── CMakeLists.txt
    ├── ft_va_cli_parse.cc
    ├── ft_va_cmd_deser.cc
    ├── regression/
    │   ├── api/
    │   ├── opt_serialize/
    │   ├── parser/
    │   ├── socket_client/
    │   └── socket_server/
    └── security/
        ├── common/
        └── stress/
```

`Fuzz/CMakeLists.txt` 通过 CMake 的 `CMAKE_PROJECT_INCLUDE` 注入测试目标，并在生产库目标建立后加载 Fuzz 内的测试目录。因此构建这些测试不需要修改项目根目录或 `test/` 目录中的任何 CMake 文件。`regression/` 不复制上游已有测试：Fuzz 回归目标直接复用 `test/cli/**` 和 `test/vasd/api/**` 的原测试源码，Fuzz 目录只保留 parser、socket_server 和 API 的新增边界用例；`security/` 保留全部 19 个压力接口测试。这样既不修改原测试目录，也避免同一份测试在两个目录维护。

Fuzz harness、driver、security 压力实现和新增回归边界文件统一使用 `.cc` 扩展名，避免被上游测试 GLOB 自动收集。Fuzz CMake 显式编译新增边界文件，并直接引用上游 `test/cli/**`、`test/vasd/api/**` 原测试文件；因此上游已有测试只有一份，普通 `BUILD_TESTS=ON` 保持原版测试集合，Fuzz 构建在同一份上游测试之上增加边界、压力和 DTFuzz 目标。普通上游构建和 Fuzz 构建使用独立构建目录。

## 2. 测试设计

### 2.1 目标与测试层次

测试目标是检查 virt-awaresched 对外 API、CLI 和 Unix Domain Socket（UDS）相关入口，在边界/异常输入、故障注入及高重复调用下是否出现崩溃、挂起或测试错误。固定输入的重复压力测试与变异式 fuzz 分开描述，不将压力循环称为随机 fuzz。

| 层级 | 被测范围 | 输入/故障模型 | 历史记录的通过条件 |
|---|---|---|---|
| E1 | API `SocketMsgHandler` | 真实临时 UDS、8 类命令语料 | 30,000,000 次或 3 小时；无 XML failure/error；真实 UDS，不 mock socket syscall |
| E2 | `ft_va_cmd_deser` harness | DTFuzz 变异输入 | 30,000,000 次或 3 小时；无 crash/hang |
| E3–E5 | API `SetConfig`、`Query`、`ReAssign` | 合法、空值、未知键、缺键、非法值、超长、嵌入 NUL、确定性随机等 map 画像 | 每接口 30,000,000 次或 3 小时；记录输入画像与类别掩码 |
| E6 | CLI parser / `MainExecuteProcess` | argv 边界及异常输入；独立执行入口 | 每接口达到上述门槛；不能以子函数结果代替执行入口结果 |
| E7–E8 | CLI callback/config | 攻击画像进入 callback/config 边界 | 每接口达到上述门槛；记录画像版本和类别掩码 |
| E9 | Socket client/server | Connect/Receive 故障画像及 Send/Accept/Receive 语料；另测真实 UDS 链路 | 每接口达到上述门槛；mock 只用于标明的故障用例，真实链路用例不 mock socket syscall |

### 2.2 输入画像和证据字段

攻击型用例应覆盖合法值、空值/空边界、未知或缺失键、非法值、超长值、嵌入 NUL 与确定性随机值；随机输入需固定 seed 以支持重放。逐项证据设计字段包括：

- `attack_profile_version` 与 `attack_categories_mask`（记录中约定 `0x0f`、`0xdf` 或 `0xff`）；
- `stress_count`、`stress_elapsed_ms`、测试失败数；
- 使用系统调用替身时的替身范围与 preflight 次数；
- 测试源码/二进制快照标识，以及结果文件哈希。

### 2.3 覆盖率与 sanitizer 采集原则

普通压力、覆盖率和 ASan/UBSan 应使用相互隔离的构建目录与结果目录。覆盖率应在同一源码/二进制快照下清理旧 `.gcda` 后重新采集；出现 gcov checksum mismatch 时该次采集无效。覆盖率范围按历史设计限定为 `src/cli/parser`、`src/cli/opt_serialize`、`src/cli/socket`、`src/vasd/api`，分别报告模块覆盖率及合计行覆盖率。Sanitizer 用例需要记录执行数或运行时间、crash/hang 数，并确认无 ASan/UBSan 报告。

### 2.4 建议复验门禁

完整复验应在确定的源码 SHA 上依次完成：编译与单元测试；19 个压力接口及其 XML/日志、次数、失败数和输入画像核验；清洁构建下覆盖率采集与阈值检查；两个 harness 的 sanitizer 长跑；最后生成不可覆盖的 manifest，记录源码 SHA、工作树状态及证据哈希。任何缺失证据、覆盖率快照不一致或 sanitizer 报告都应判为未通过/未完成，而不是以 smoke 替代正式门槛。

执行前仍需确认目标源码与当前测试接口兼容，并按本节在明确的源码 SHA 上采集完整证据。本文不将历史结果等同于本次 PR 提交后的重跑结果。

### 2.5 复验指导手册（Linux / openEuler ARM64）

本节给出从干净源码快照复跑本 PR 测试的最短路径。命令在仓库根目录执行；将 `build-arm64-check` 保留为本次专用构建目录，依赖目录按目标机实际位置调整。运行前记录 `git rev-parse HEAD`、`git status --short`、`uname -a`、`g++ --version`，并确认测试快照不包含未提交的生产源码改动。

**1. 配置并编译定向目标**（覆盖率构建；`SKIP_RUN_TESTS=ON` 只编译，不会在 build 阶段隐式运行测试）：

```bash
cmake -S virt-awaresched -B build-arm64-check \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTS=OFF -DFT_COVERAGE=ON -DENABLE_COVERAGE=ON \
  -DCMAKE_PROJECT_INCLUDE="$PWD/virt-awaresched/test/Fuzz/CMakeLists.txt" \
  -DDEPS_DIR=/path/to/prepared/deps \
  -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/path/to/googletest-src
cmake --build build-arm64-check --target \
  fuzz_tests -j8
mkdir -p build-arm64-check/coverage
```

如果构建依赖已由项目正常解析，可省略最后两项依赖路径参数。所有测试可执行文件都位于 `build-arm64-check/bin/`。`BUILD_TESTS=OFF` 避免加载既有 `test/CMakeLists.txt`；Fuzz 自己的 CMake 入口仍会构建完整回归、压力和 fuzz 目标。

**2. 运行与源码回退相关的定向测试**：

```bash
./build-arm64-check/bin/VirtAwareSched_parser_ut \
  --gtest_output=xml:build-arm64-check/coverage/parser-detail.xml

VAS_STRESS_COUNT=10000 VAS_STRESS_SECONDS=300 \
  ./build-arm64-check/bin/VirtAwareSched_security_ut \
  --gtest_filter='TestStressArgParse.*' \
  --gtest_output=xml:build-arm64-check/coverage/argparse-detail.xml

./build-arm64-check/bin/ft_va_cli_parse \
  --runs=10000 --time=300 --hang-timeout=5000 --seed=20260924 \
  --crash-dir=build-arm64-check/parser-crashes \
  --corpus=build-arm64-check/parser-corpus
```

上述低次数参数仅用于快速定向复验，不满足正式压力门槛。完整压力测试应在专用环境中取消 `VAS_STRESS_COUNT` / `VAS_STRESS_SECONDS` 覆盖，让每项按默认最多 30,000,000 次或 10,800 秒（先到者）执行；不要在未确认耗时和资源影响前对全部接口直接启动长跑。`ft_va_cli_parse` 的 10,000 次同样是 smoke，不替代 sanitizer 长跑。

**3. 采集覆盖率并核对**：`ft_coverage.sh` 会建立独立覆盖率构建，运行 parser、opt_serialize、socket_server、socket_client、API 完整回归、19 项 CI 次数压力测试及两个 harness smoke，再由 `gcov_summary.sh` 仅统计 `src/cli/parser`、`src/cli/opt_serialize`、`src/cli/socket`、`src/vasd/api`。脚本优先使用 `lcov`，没有 `lcov` 时使用 Python `gcovr`；四个模块逐一执行 85% 行覆盖率门槛，任何模块不足都会返回非零。

```bash
FT_BUILD_DIR="$PWD/build-arm64-check" \
  bash virt-awaresched/test/Fuzz/scripts/ft_coverage.sh
```

快速运行两个 harness 时可设置 `FT_RUNS` 和 `FT_SECONDS`；正式验收不设置覆盖值，脚本默认每个 harness 30,000,000 次或 10,800 秒，先到者为准：

```bash
bash virt-awaresched/test/Fuzz/scripts/run_harness.sh
```

脚本按循环序号为两个 harness 分配不同 seed：第一个使用 `FT_SEED`，第二个使用 `FT_SEED+1`；日志中的 `base_seed` 是实际传给 harness 的值，便于复现且避免两个目标因名称长度相同而意外共用变异序列。

**4. 判断与归档**：确认 GoogleTest XML 的 `failures=0`、`errors=0`，执行数达到本次指定门槛，DTFuzz 的 `execs_done` 达到配置次数且 `saved_crashes=0`、`saved_hangs=0`，并检查 sanitizer/gcov 输出。保存命令、退出码、XML、fuzz 摘要、覆盖率原始数据和环境信息到仓库外的证据目录，生成 SHA-256 清单；报告中区分历史结果、定向 smoke 和正式长跑。不要把临时 XML、`.gcda`、fuzz corpus/crash 文件或本地生成的测试二进制加入提交。

本次提交的 CLI 行宽单测使用正的剩余行宽；parser DTFuzz harness 对 `PrintWithLineLimit` 的私有直接调用仅使用 `lineLimit >= 47`（`indentSize=46`），真实帮助路径本身只传入大于 78 的行宽。2026-09-24 在原版生产源码上复现：人为直接传入 `lineLimit=1` 可在 `vas_cli_parse.cpp:443` 触发 `std::string::operator[]` 越界断言，`lineLimit <= 46` 也可能使后续行宽不前进。因此这一已知边界被明确排除在本次通过性 smoke 之外；测试通过不代表生产实现已修复该边界，也不能外推到这些参数。该边界需要在生产实现获准修改后再加入可运行的回归断言。

## 3. openEuler ARM64 历史测试环境

| 项目 | 测试记录 |
|---|---|
| OS / 架构 | openEuler 24.03 LTS-SP3 / aarch64 |
| CPU / 内核 | Kunpeng-920，128 CPUs / `6.6.0-132.0.0.111.oe2403sp3.aarch64` |
| 编译器 / 构建 | GCC 12.3.1；Debug，`BUILD_TESTS=ON`（上游 DT）；Fuzz 复验使用 `CMAKE_PROJECT_INCLUDE=.../test/Fuzz/CMakeLists.txt`，不使用未定义的 `ENABLE_FT` 开关 |
| libvirt | `9.10.0-34.oe2403sp3`；libvirtd sockets active，`qemu:///system`、capabilities、nodeinfo 可用 |
| cgroup | v1；systemd、cpu/cpuacct、memory、cpuset、devices、pids 等控制器已挂载 |
| 单元测试 | 普通上游 `VirtAwareSched_ut`：184/184 passed；Fuzz 目标 91/91 passed |

环境检查时 `virsh list --all` 为空，未配置 guest domain；日志中有可选 D-Bus 电源管理及 `/sys/kernel/tmm/memory_info` 探测缺失提示。libvirtd 与 `qemu:///system` 可用，但这些测试不构成真实 guest 生命周期验证。

## 4. 19 个接口 ARM64 最终压力结果

2026-09-28 在 openEuler ARM64 目标机上以默认正式门槛重新运行全部 19 项。每项均完成 30,000,000 次调用后结束，没有触发单项 10,800 秒上限；GoogleTest 总耗时 12,193.034 秒，XML 记录 `tests=19`、`failures=0`、`errors=0`、`disabled=0`。下表为最终 XML/日志中的单项耗时。

| 接口用例 | 耗时 | 结果 |
|---|---:|---|
| `TestStressApi.testStressSetConfig` | 115.023 s | PASS |
| `TestStressApi.testStressQueryCpuAffinityInfo` | 161.861 s | PASS |
| `TestStressApi.testStressReAssign` | 214.854 s | PASS |
| `TestStressApi.testStressDeSerialize` | 96.179 s | PASS |
| `TestStressApi.testStressSocketMsgHandler` | 283.728 s | PASS |
| `TestStressArgParse.testStressMainExecuteProcess` | 1,280.535 s | PASS |
| `TestStressArgParse.testStressCliSetConfFunc` | 304.457 s | PASS |
| `TestStressArgParse.testStressCliQueryAffinityFunc` | 305.447 s | PASS |
| `TestStressArgParse.testStressCliOptReassignFunc` | 306.012 s | PASS |
| `TestStressArgParse.testStressCliOptRecoverFunc` | 738.236 s | PASS |
| `TestStressArgParse.testStressCliSetServerConfFunc` | 201.099 s | PASS |
| `TestStressArgParse.testStressRegisterServerModuleSDK` | 775.075 s | PASS |
| `TestStressSocket.testStressRealUdsFullChain` | 759.853 s | PASS |
| `TestStressSocket.testStressClientConnectToServer` | 1,607.193 s | PASS |
| `TestStressSocket.testStressClientSendMessage` | 439.180 s | PASS |
| `TestStressSocket.testStressClientReceiveMessage` | 746.518 s | PASS |
| `TestStressSocket.testStressServerReceiveMessage` | 540.943 s | PASS |
| `TestStressSocket.testStressServerAcceptAndSend` | 1,055.670 s | PASS |
| `TestStressSocket.testStressServerStartServer` | 2,261.161 s | PASS |

`ClientConnectToServer` 约 26.8 分钟，`ServerStartServer` 约 37.7 分钟；两项均完成且未超过 3 小时。mock 用例的结果仅适用于相应入口及故障画像，不外推为真实网络或 libvirt 全链路；`RealUdsFullChain` 对应真实临时 UDS 链路。最终 XML SHA-256 为 `16c4eeb6cf235558c71b36b536581fb554006ccec44533626b130cd7fca96cea`，完整日志 SHA-256 为 `06dcec2469cfee4ddc09586d2c503eaf858af5f9e3beb056fd16395141d0d54d`。

## 5. 覆盖率、Sanitizer 与目标机 smoke

### 5.1 历史记录

- 本地同源码快照覆盖率记录：总计 **90.1%（609/676 行）**；`cli/parser` **92.5%**、`cli/opt_serialize` **100.0%**、`cli/socket` **85.8%**、`vasd/api` **85.9%**。记录称四个模块均达到至少 85%，且无 gcov checksum mismatch。该数据是本地 coverage 构建结果，不是 ARM64 目标机覆盖率。
- 本地 ASan+UBSan 记录：`ft_va_cmd_deser` 与 `ft_va_cli_parse` 各 30,000,000 次，记录为 0 crash、0 hang、无 ASan/UBSan 报告。
- ARM64 目标机历史 DTFuzz smoke 记录：两个 harness 各 10,000 次，0 saved crashes、0 saved hangs。Smoke 只作为目标平台构建/启动检查，不替代 30,000,000 次或 3 小时的 sanitizer 长跑门槛。

### 5.2 2026-09-24 ARM64 定向复验

在原版生产源码快照上重新构建本 PR 测试代码（openEuler 24.03 LTS-SP3，aarch64，GCC 12.3.1；Debug、`BUILD_TESTS=ON`、`ENABLE_COVERAGE=ON`）。只复验与之前源码回退直接相关的 CLI parser 与 `vasctl` 参数解析入口，没有重跑未受影响的 19 项长时压力测试或 libvirt 全套测试；该定向测试不依赖 libvirtd，因此没有改变目标机服务状态。

| 检查 | ARM64 结果 |
|---|---|
| `VirtAwareSched_parser_ut` | 32/32 passed（包含新增的非法/重复选项边界用例） |
| `TestStressArgParse.*` 定向低次数运行 | 7/7 passed，`VAS_STRESS_COUNT=10000`、`VAS_STRESS_SECONDS=300` |
| `ft_va_cli_parse` DTFuzz smoke | 10,000 executions；0 saved crashes、0 saved hangs；seed `20260924` |
| `src/cli/parser/vas_cli_parse.cpp` 覆盖率 | 240/277 executable lines，86.64% |
| `src/vasctl/arg_parse/vasctl_arg_parse.cpp` 覆盖率 | 103/111 executable lines，92.79% |

覆盖率只针对上述定向执行实际采样到的两个源文件；本次没有重跑全部模块测试，故不把这些数字表述为 `cli/parser` 模块整体覆盖率，也不将历史模块覆盖率数字更新为本次结果。该段记录属于历史测试版本，不作为当前 PR 的源码引用；当前 PR 以 PR 页面显示的最新 head SHA 为准。

### 5.3 2026-09-28 Fuzz 独立目录 ARM64 复验

在 openEuler 24.03 LTS-SP3 aarch64、GCC 12.3.1、CMake 3.27.9 环境中，分别使用干净的 `BUILD_TESTS=ON` 上游构建和 `BUILD_TESTS=OFF` + `CMAKE_PROJECT_INCLUDE=test/Fuzz/CMakeLists.txt` 的 Fuzz 构建。普通上游目标只收集原版 `*.cpp` 测试并通过 184/184；Fuzz 回归目标直接复用上游 `test/cli/**`、`test/vasd/api/**` 源码，并由 Fuzz CMake 显式加入 7 个 `.cc` 边界用例。复验使用上游生产源码基线 `08d1501`；生产源码、生产源码注释、项目根 CMake 及原 `test/` 目录均未修改。目标机的 libvirt 9.10.0 socket 已启用，`qemu:///system` 与 nodeinfo 可访问；cgroup v1 控制器保持可用。

| 检查 | ARM64 结果 |
|---|---|
| 普通上游 `VirtAwareSched_ut` | 原版 184/184 passed；不收集 Fuzz 新增边界文件 |
| Fuzz 回归与压力 smoke | 91/91 passed：parser 32、opt_serialize 2、socket_server 14、socket_client 6、API 18、安全压力 19 |
| `ft_va_cmd_deser` 正式长跑 | 30,000,000 executions；600 s；49,956 exec/s；0 saved crashes；0 saved hangs；peak RSS 3,212 KiB；peak FD 4 |
| `ft_va_cli_parse` 正式长跑 | 30,000,000 executions；1,449 s；20,694 exec/s；0 saved crashes；0 saved hangs；peak RSS 3,248 KiB；peak FD 4 |
| `cli/parser` 行覆盖率 | 94.0%（363/386） |
| `cli/opt_serialize` 行覆盖率 | 100.0%（29/29） |
| `cli/socket` 行覆盖率 | 89.3%（167/187） |
| `vasd/api` 行覆盖率 | 85.9%（79/92） |
| 四模块合计行覆盖率 | 91.9%（638/694） |

本次覆盖率由 gcovr 7.2 从清理后的 `.gcda` 重新采集，四个模块均通过 85% 行覆盖率门槛。覆盖率流程中的 19 项安全压力测试使用 `VAS_CI=1` 快速次数，仅验证目录迁移后的构建、链接和入口可运行；随后另行取消次数/时间覆盖，完成第 4 节所列的正式 19 接口长跑。两个 DTFuzz harness 同样按 30,000,000 次或 10,800 秒先到者为准执行，均以达到 30,000,000 次结束。最终日志 SHA-256 分别为：`ft_va_cmd_deser` `b0c80e6c791462ed262d1b7b6a77fb5e63ed40f16462e6cf19cd06892f01edf0`，`ft_va_cli_parse` `4d3d347b127594bbddf909520c7b1e3faebfd4022eb7f725d6e9eff2293e3e32`。

PR 最新 CI 触发任务已完成，SCA、代码检查、许可证/版权检查、x86_64、aarch64、pre-commit、DT 与 Issue #106 关联检查均为 `SUCCESS`；复验人员应以 PR 最新 head 对应的 CI 评论为准，不使用历史 force-push 前的任务编号或 SHA。

## 6. 证据限制与解读

5.1 和 5.2 保留既有测试记录，5.3 与第 4 节是本次 Fuzz 独立目录后的 ARM64 最终复验。逐次 XML、日志、覆盖率 JSON、sanitizer 输出和环境记录按要求保存在目标机构建目录而未随 PR 提交，因此仓库内报告不能独立复算每个原始计数；报告给出最终 XML、长跑日志的 SHA-256，便于在目标机证据目录核验。两个 harness 和 19 项接口均已达到 30,000,000 次正式门槛，不再以 smoke 结果代替正式长跑。

本 PR 的变更范围限定在 `virt-awaresched/test/Fuzz/`。若评审要求将可审计原始产物纳入长期归档，应从目标机证据目录单独归档，不应把临时 XML、`.gcda`、corpus、crash 目录或测试二进制加入源码提交。
