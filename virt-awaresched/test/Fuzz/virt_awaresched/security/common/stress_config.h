/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 *
 * VSched is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *      http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef STRESS_CONFIG_H
#define STRESS_CONFIG_H

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <string>

namespace vas::security::test {

struct StressRunInfo {
    uint64_t executions{};
    uint64_t elapsedMs{};
    uint64_t countLimit{};
    uint32_t secondsLimit{};
};

inline thread_local StressRunInfo g_lastStressRun{};

// 默认压力循环次数：3000 万（满足单接口 3000 万次要求）
constexpr uint64_t DEFAULT_STRESS_COUNT = 30000000ULL;
// 默认压力最长时长：3 小时（10800 秒）
constexpr uint32_t DEFAULT_STRESS_SECONDS = 3U * 3600U;
// CI 默认小循环次数
constexpr uint64_t CI_STRESS_COUNT = 10000ULL;
// CI 默认小循环时长（秒）
constexpr uint32_t CI_STRESS_SECONDS = 10U;

// 读取 VAS_STRESS_COUNT 环境变量；未设置时若 VAS_CI=1 返回 CI 默认值，否则返回 3000 万
inline uint64_t GetStressCount()
{
    if (const char *env = std::getenv("VAS_STRESS_COUNT"); env != nullptr) {
        try {
            const auto v = std::stoull(env);
            return v == 0 ? DEFAULT_STRESS_COUNT : v;
        } catch (...) {
            return DEFAULT_STRESS_COUNT;
        }
    }
    if (const char *ci = std::getenv("VAS_CI"); ci != nullptr && std::string(ci) == "1") {
        return CI_STRESS_COUNT;
    }
    return DEFAULT_STRESS_COUNT;
}

// 读取 VAS_STRESS_SECONDS 环境变量；未设置时若 VAS_CI=1 返回 10s，否则返回 3 小时
inline uint32_t GetStressSeconds()
{
    if (const char *env = std::getenv("VAS_STRESS_SECONDS"); env != nullptr) {
        try {
            const auto v = std::stoul(env);
            return v == 0 ? DEFAULT_STRESS_SECONDS : static_cast<uint32_t>(v);
        } catch (...) {
            return DEFAULT_STRESS_SECONDS;
        }
    }
    if (const char *ci = std::getenv("VAS_CI"); ci != nullptr && std::string(ci) == "1") {
        return CI_STRESS_SECONDS;
    }
    return DEFAULT_STRESS_SECONDS;
}

// 压力测试执行器：循环执行 fn，直到达到次数上限或时长上限（取先到者）
// fn 签名为 void()，内部自行断言
template <typename Fn>
uint64_t RunStress(const std::string &name, Fn fn)
{
    (void)name;
    const uint64_t maxCount = GetStressCount();
    const uint32_t maxSeconds = GetStressSeconds();
    const auto start = std::chrono::steady_clock::now();
    const auto deadline = start + std::chrono::seconds(maxSeconds);

    uint64_t done = 0;
    while (done < maxCount) {
        fn();
        ++done;
        // 每 1024 次检查一次 deadline，兼顾时间精度与性能开销
        if ((done & 0x3FFU) == 0U && std::chrono::steady_clock::now() >= deadline) {
            break;
        }
    }
    const auto elapsed = std::chrono::steady_clock::now() - start;
    g_lastStressRun = StressRunInfo{
        done,
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()),
        maxCount,
        maxSeconds,
    };
    return done;
}

inline const StressRunInfo &GetLastStressRunInfo()
{
    return g_lastStressRun;
}

inline bool StressAcceptanceReached()
{
    const auto &info = GetLastStressRunInfo();
    return info.executions >= info.countLimit || info.elapsedMs >= static_cast<uint64_t>(info.secondsLimit) * 1000ULL;
}

} // namespace vas::security::test

#endif // STRESS_CONFIG_H
