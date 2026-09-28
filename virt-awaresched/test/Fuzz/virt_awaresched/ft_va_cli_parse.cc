/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 *
 * virt-awaresched is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

/*
 * DT Fuzz harness: virt-awaresched CLI argv 解析攻击面
 *
 * 攻击面:
 *   vas::cli::framework::VasCliParse::GetInstance().SdkCliParse(args)
 *   vasctl 将 argv 交给框架解析（命令匹配、选项拆分、帮助打印、超长值校验）。
 *   恶意 argv（超长 option value、unknown option、缺值、help 触发、空 token）进入不同分支。
 *   本 harness 进程内注册少量命令后直接调用 SdkCliParse + getter 消费结果。
 *
 * 输入布局:
 *   字节流以 '\n' 切分为 argv tokens，前面补齐程序名 "vasctl"（index 0 不参与解析）。
 *   每次执行前 Reset() + 重新注册命令，避免单例状态在 fork 子进程批次内跨迭代串扰。
 */

#define _XOPEN_SOURCE 700 // posix_openpt/grantpt/unlockpt/ptsname 声明

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <streambuf>
#include <string>
#include <vector>

#include "cmd.h"
#include "vas_cli_parse.h"
#include "vas_cli_reg_builder.h"

using vas::cli::framework::SdkCmdInfoBuilder;
using vas::cli::framework::VasCliParse;
using vas::cli::framework::VasCliSdkCmdInfo;
using vas::cli::framework::VasCliSdkResult;

namespace {

volatile uint64_t g_sink = 0;

void SinkBytes(const void *p, size_t n)
{
    const auto *b = static_cast<const uint8_t *>(p);
    uint64_t acc = 0;
    if (n > 256) {
        n = 256;
    }
    for (size_t i = 0; i < n; ++i) {
        acc = acc * 31 + b[i];
    }
    g_sink ^= acc;
}

// 静音 std::cout/std::cerr：解析错误路径会高频打印，30M 次会产生海量日志拖慢执行。
// 只替换 C++ 流缓冲，不动 fd 1，driver 的 fprintf(stdout) 进度输出不受影响。
class NullBuf : public std::streambuf {
protected:
    int overflow(int c) override
    {
        return c;
    }
    std::streamsize xsputn(const char *, std::streamsize n) override
    {
        return n;
    }
};
NullBuf g_nullBuf;
struct StreamMuter {
    StreamMuter()
    {
        std::cout.rdbuf(&g_nullBuf);
        std::cerr.rdbuf(&g_nullBuf);
    }
} g_muter;

// 伪终端（可逆）：惰性创建 pty，但不永久 dup2 到 stdout。每次输入按 DecideTty 决定是否
// 进入 tty 环境（dup2 slave -> stdout），执行完再恢复原始 stdout，从而让同一 fork 子进程内
// 不同输入独立地在「真实终端」与「重定向管道」两种环境间切换：
//   - tty：GetTerminalWidth / ParsePrtHelpInfo 的 ioctl(TIOCGWINSZ) 成功，触达帮助排版路径。
//   - 非 tty：ioctl 失败，触达 ParsePrtHelpInfo 行 326 的 cerr（ioctl 失败分支）。
// 仅在子进程内惰性初始化，不影响父进程 stdout（父进程汇总仍走管道）。
int g_ptyMaster = -1;  // 伪终端 master（保持打开，避免 pty 关闭被拆除）
int g_ptySlave = -1;   // 伪终端 slave（dup2 进 stdout 时使用）
int g_origStdout = -1; // 原始 stdout 备份（每次 tty 结束恢复）

void EnsurePty()
{
    static bool initOnce = []() {
        int master = posix_openpt(O_RDWR | O_NOCTTY);
        if (master < 0) {
            return false;
        }
        if (grantpt(master) != 0 || unlockpt(master) != 0) {
            close(master);
            return false;
        }
        const char *name = ptsname(master);
        if (name == nullptr) {
            close(master);
            return false;
        }
        int slave = open(name, O_RDWR);
        if (slave < 0) {
            close(master);
            return false;
        }
        int orig = dup(STDOUT_FILENO);
        if (orig < 0) {
            close(slave);
            close(master);
            return false;
        }
        g_origStdout = orig;
        g_ptySlave = slave;
        g_ptyMaster = master;
        return true;
    }();
    (void)initOnce;
}

void EnterTty()
{
    if (g_ptySlave >= 0) {
        (void)dup2(g_ptySlave, STDOUT_FILENO);
    }
}

void ExitTty()
{
    if (g_origStdout >= 0) {
        (void)dup2(g_origStdout, STDOUT_FILENO);
    }
}

// tty 决策：对三类会触发 ParsePrtHelpInfo 的输入做确定性分区，其余走 FNV-1a 整体哈希自然混合：
//   - size==0（无参数）→ 非 tty：命中 PrtHelp 的 size<=NO_1 分支 + ParsePrtHelpInfo 的 ioctl 失败 cerr。
//   - "-h"      → tty：配空注册，命中 ParsePrtHelpInfo 的 sdkCmdInfo.empty 分支。
//   - "--help"  → tty：配完整注册，命中 ParsePrtHelpInfo 逐命令 Usage 打印主循环。
// 均匀覆盖真实世界里 vasctl 既可交互终端又可重定向输出的两种环境。
bool DecideTty(const uint8_t *data, size_t size)
{
    if (size == 0) {
        return false;
    }
    if (size == 2 && data[0] == '-' && data[1] == 'h') {
        return true;
    }
    if (size == 6 && data[0] == '-' && data[1] == '-' && data[2] == 'h' && data[3] == 'e' && data[4] == 'l' &&
        data[5] == 'p') {
        return true;
    }
    uint32_t h = 2166136261u; // FNV-1a offset basis
    for (size_t i = 0; i < size; ++i) {
        h ^= data[i];
        h *= 16777619u; // FNV-1a prime
    }
    return (h & 1u) != 0;
}

// 宽度选择器：窄(20/78)触发分行+Tab 排版(PrintWithWordWrap/HandleTab)，宽(79/120)触发
// PrintWithLineLimit 的真实帮助调用路径。更窄的边界值由下方 DirectFuzzLineLimit 直接验证。
void SetTerminalWidth(const uint8_t *data, size_t size)
{
    static const int kWidths[] = {20, 78, 79, 120};
    struct winsize ws {
    };
    ws.ws_row = 50;
    ws.ws_col = static_cast<unsigned short>(kWidths[(size > 0 ? data[0] : 0) % 4]);
    if (g_ptyMaster >= 0) {
        (void)ioctl(g_ptyMaster, TIOCSWINSZ, &ws);
    }
}

// PrintWithLineLimit 是帮助输出的内部边界函数。原版生产实现要求行宽大于
// indentSize=46，否则后续行宽不前进或越界；真实帮助调用点只会传入 >78。
// 源码保持原样时仅覆盖可前进的 47 及以上行宽，不把已知不支持的私有函数参数
// 混入可交付的 smoke。<=46 的问题作为已知生产边界限制单独记录。
void DirectFuzzLineLimit(VasCliParse &parser, const uint8_t *data, size_t size)
{
    static const int kLineLimits[] = {47, 48, 64, 78, 79, 120};
    const int lineLimit = kLineLimits[(size > 0 ? data[0] : 0) % (sizeof(kLineLimits) / sizeof(kLineLimits[0]))];
    const size_t textLen = size > 256 ? 256 : size;
    const std::string text(reinterpret_cast<const char *>(data), textLen);
    parser.PrintWithLineLimit(text, lineLimit, "\n");
}

// 占位回调：对齐 vasd/vasctl 真实注册（每条命令都会设置执行回调），此处仅用于覆盖
// VasCliRegBuilder::SetVasCliSdkCmdFun，SdkCliParse 不会真正调用它。
VasCliSdkResult NoopCmdFun(const std::map<std::string, std::string> &)
{
    return VasCliSdkResult{};
}

// 一次性构造注册命令（与 cmd.h 定义的真实 CLI 命令/选项一致）
std::vector<VasCliSdkCmdInfo> BuildCommands()
{
    std::vector<VasCliSdkCmdInfo> v;
    v.push_back(SdkCmdInfoBuilder()
                    .SetCommand("set")
                    .SetType("config")
                    .SetDesc(SET_CONFIG_DES)
                    .AddParam(SCHED_POLICY_SHORT, SCHED_POLICY, SCHED_POLICY_DES)
                    .SetVasCliSdkCmdFun(NoopCmdFun)
                    .Build());
    v.push_back(SdkCmdInfoBuilder()
                    .SetCommand("query")
                    .SetType("affinity")
                    .SetDesc(QUERY_AFFINITY_DES)
                    .AddParam(AFFINITY_SCOPE_SHORT, AFFINITY_SCOPE, AFFINITY_SCOPE_DES)
                    .SetVasCliSdkCmdFun(NoopCmdFun)
                    .Build());
    v.push_back(SdkCmdInfoBuilder()
                    .SetCommand("opt")
                    .SetType("reassign")
                    .SetDesc(REASSIGN_DES)
                    .AddParam(REASSIGN_SCOPE_SHORT, REASSIGN_SCOPE, REASSIGN_SCOPE_DES)
                    .SetVasCliSdkCmdFun(NoopCmdFun)
                    .Build());
    v.push_back(SdkCmdInfoBuilder()
                    .SetCommand("opt")
                    .SetType("recover")
                    .SetDesc(RECOVER_DES)
                    .AddParam(RECOVER_VM_SHORT, RECOVER_VM, RECOVER_VM_DES)
                    .SetVasCliSdkCmdFun(NoopCmdFun)
                    .Build());
    // 无选项命令：框架明确支持（SdkCmdInfoRegister 的空 params 分支、帮助空选项展示、
    // SdkCmdOptParse 的"不支持选项"报错分支都依赖此类命令存在）
    v.push_back(SdkCmdInfoBuilder()
                    .SetCommand("version")
                    .SetType("info")
                    .SetDesc("Show version information")
                    .SetVasCliSdkCmdFun(NoopCmdFun)
                    .Build());
    return v;
}

std::vector<VasCliSdkCmdInfo> &Commands()
{
    static std::vector<VasCliSdkCmdInfo> cmds = BuildCommands();
    return cmds;
}

// 以 '\n' 切分发 argv，令牌数上限定为 64，避免超长串生成海量参数
std::vector<std::string> SplitToArgs(const uint8_t *data, size_t size)
{
    std::vector<std::string> args;
    args.reserve(64);
    args.emplace_back("vasctl"); // argv[0]，SdkCliParse 不读取它
    if (size == 0) {
        return args; // 无参数 → args.size()==1，命中 PrtHelp 的 size<=NO_1 分支
    }
    std::string cur;
    size_t cnt = 0;
    for (size_t i = 0; i < size && cnt < 64; ++i) {
        char c = static_cast<char>(data[i]);
        if (c == '\n') {
            args.emplace_back(std::move(cur));
            cur.clear();
            ++cnt;
        } else {
            cur += c;
        }
    }
    if (cnt < 64) {
        args.emplace_back(std::move(cur));
    }
    return args;
}

bool WriteSeed(const std::string &dir, int idx, const std::string &content)
{
    char path[512];
    std::snprintf(path, sizeof(path), "%s/seed_%d", dir.c_str(), idx);
    FILE *f = std::fopen(path, "wb");
    if (f == nullptr) {
        return false;
    }
    const size_t n = std::fwrite(content.data(), 1, content.size(), f);
    std::fclose(f);
    return n == content.size();
}

} // namespace

// 语料目录为空时由 driver 调用，生成结构合法的 argv 种子（覆盖帮助/选项/缺值/未知/超长）
extern "C" int FtGenSeeds(const char *corpusDir)
{
    std::string longValue = "set\nconfig\n--sched-policy\n" + std::string(2048, 'A');
    const char *seeds[] = {
        "set\nconfig\n--sched-policy\ndynamicAffinity",
        "set\nconfig\n--sched-policy\naffinity",
        "set\nconfig\n-sp\ndynamicAffinity",
        "query\naffinity\n--scope\nall",
        "opt\nreassign\n-s\nuuid\n<uuid>",
        "opt\nrecover\n-v\nall",
        "-h",
        "--help",
        "set\nconfig\n-h",
        "set\nconfig\n--unknown\nvalue",
        "set\nconfig\n--sched-policy",
        longValue.c_str(),
        "set\nconfig\nfoo",
        "set\nconfig\n-sp\na\n-sp\nb",
        "set\nconfig\n--sched-policy\na\n--sched-policy\nb",
        "query\naffinity",
        "set",
        "version\ninfo\nextra",
        "version\ninfo\n-h",
    };
    int n = 0;
    for (size_t i = 0; i < sizeof(seeds) / sizeof(seeds[0]); ++i) {
        if (WriteSeed(corpusDir, static_cast<int>(i), seeds[i])) {
            ++n;
        }
    }
    return n;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // tty 由 DecideTty 决定：tty 命中帮助展示的宽/窄排版（ParsePrtHelpInfo 主循环 /
    // CommandTypeParamsHelpInfo / PrintWithLineLimit / HandleTab），非 tty 命中
    // GetTerminalWidth / ParsePrtHelpInfo / ParseOneCommandPrtHelpInfo 的"取终端尺寸失败"分支。
    // pty 可逆（EnterTty/ExitTty），每个 fork 子进程内不同输入独立切换，均匀混合两种环境。
    const bool usePty = DecideTty(data, size);
    if (usePty) {
        EnsurePty();
        SetTerminalWidth(data, size);
        EnterTty();
    }

    auto &parser = VasCliParse::GetInstance();
    // 关键：单例状态在 fork 子进程的批次内跨迭代累积（needPrtHelp/inputOptionMap），
    // 每次重置并重新注册，保证每轮独立、覆盖不塌缩。
    parser.Reset();
    // 短帮助 "-h"（仅两字节、无换行）走「空注册」：覆盖 VasCliRegisterSdkCmdInfo 空向量
    // 提前返回分支与 ParsePrtHelpInfo 的 sdkCmdInfo.empty 分支（"No commands registered"）。
    // 长帮助 "--help" 及其余输入完整注册，保留 argv 解析主攻击面与逐命令 Usage 打印路径。
    // 空输入也走这里（非空），经 PrtHelp 的 size<=NO_1 分支触发 ParsePrtHelpInfo 的 ioctl 失败 cerr。
    if (size == 2 && data[0] == '-' && data[1] == 'h') {
        std::vector<VasCliSdkCmdInfo> empty;
        VasCliParse::VasCliRegisterSdkCmdInfo(empty);
    } else {
        VasCliParse::VasCliRegisterSdkCmdInfo(Commands());
    }

    const std::vector<std::string> args = SplitToArgs(data, size);
    (void)parser.SdkCliParse(args);
    DirectFuzzLineLimit(parser, data, size);
    if (usePty) {
        ExitTty();
    }

    // 消费 getter 结果，防优化
    const auto &optMap = parser.GetInputOptionMap();
    g_sink ^= optMap.size();
    for (const auto &[k, v] : optMap) {
        SinkBytes(k.data(), k.size());
        SinkBytes(v.data(), v.size());
    }
    const auto info = parser.GetSdkCommandInfo();
    SinkBytes(info.command.data(), info.command.size());
    SinkBytes(info.type.data(), info.type.size());
    g_sink ^= info.params.size();

    // 补消费其余公开 getter，覆盖 GetSdkCmdInfo / GetSdkCommandWithOptions
    g_sink ^= parser.GetSdkCmdInfo().size();
    g_sink ^= parser.GetSdkCommandWithOptions().size();
    return 0;
}
