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
 * DT Fuzz harness: virt-awaresched CLI 命令串反序列化攻击面
 *
 * 攻击面:
 *   vas::common::CmdSerialize::DeSerialize(const std::string&, CmdOption&)
 *   vasd 通过 UDS/CLI 收到命令串后交给 DeSerialize 按 ';' 分段、':' 拆键值。
 *   恶意/损坏串可伪造 option、超长键值、缺失冒号、空 token 等，本 harness 进程内
 *   直接调用 DeSerialize + 深度消费 getter 做白盒攻击测试（不触碰 Deserialize 之后的
 *   业务逻辑，该部分由 UT/ST 覆盖）。
 *
 * 输入布局:
 *   整个字节流作为 cmdStr 交给 DeSerialize。
 */

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

#include "cmd_serialize.h"

using vas::common::CmdOption;
using vas::common::CmdSerialize;

namespace {

volatile uint64_t g_sink = 0;

// 消费 getter 结果，防止编译器把数据搬运优化掉
void SinkBytes(const void *p, size_t n)
{
    const auto *b = static_cast<const uint8_t *>(p);
    uint64_t acc = 0;
    if (n > 256) { // 大串只消费头部，避免拖慢 fuzz 主循环
        n = 256;
    }
    for (size_t i = 0; i < n; ++i) {
        acc = acc * 31 + b[i];
    }
    g_sink ^= acc;
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

// 语料目录为空时由 driver 调用，生成结构合法的边界种子
extern "C" int FtGenSeeds(const char *corpusDir)
{
    const char *seeds[] = {
        "",
        "set",
        "set;config:sched-policy:dynamicAffinity",
        "set;config:sched-policy:affinity;scope:all",
        "query;affinity:scope:all",
        "opt;reassign:scope:uuid",
        "opt;recover:vm:all",
        ";",
        "a:b:c",
        "a:",
        ":b",
        "::::",
        "set;config:sched-policy:",
        "op;k:v",
        "op;k:v;x:y",
        "op;k:v:extra",
        "op;k",
        "op;",
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
    std::string cmdStr(reinterpret_cast<const char *>(data), size);
    CmdOption opt;
    (void)CmdSerialize::DeSerialize(cmdStr, opt);

    // 深度消费结果，确保反序列化 + 数据搬运路径真实执行
    SinkBytes(opt.option.data(), opt.option.size());
    uint64_t acc = opt.params.size();
    for (const auto &[k, v] : opt.params) {
        acc = acc * 31 + k.size() + v.size();
        SinkBytes(k.data(), k.size());
        SinkBytes(v.data(), v.size());
    }
    g_sink ^= acc;

    // 反向编码（Serialize）：纯字符串拼接、无重依赖，补上反序列化之外的函数覆盖
    std::string roundTrip;
    (void)CmdSerialize::Serialize(opt, roundTrip);
    SinkBytes(roundTrip.data(), roundTrip.size());
    return 0;
}