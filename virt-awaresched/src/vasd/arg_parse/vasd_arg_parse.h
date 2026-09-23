/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 *
 * VSched is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *      http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef VAS_CLI_ARG_H
#define VAS_CLI_ARG_H

#include <filesystem>
#include <mutex>
#include <string>

#include "error.h"
#include "vas_cli_reg_builder.h"

namespace vas::sched {
using namespace vas::common;
namespace fs = std::filesystem;
const fs::path PROC_CMDLINE = "/proc/cmdline";
const fs::path DYNAMIC_UTIL_THRESH_PATH = "/proc/sys/kernel/sched_util_low_pct";
const std::string DYNAMIC_AFFINITY_ENABLE_KEY = "dynamic_affinity=enable";

class VasdArgParse {
public:
    static bool smt;
    static std::string schedPolicy;
    static uint16_t dynamicAffinityUtilThresh;
    static std::string skippedCPUSet;
    static bool rangeAffinity;

    // Thread-safe accessors for the static config: all cross-thread access
    // must go through them to avoid data races (std::string tearing).
    static bool IsSmt();
    static void SetSmt(bool value);
    static std::string GetSchedPolicy();
    static void SetSchedPolicy(const std::string &policy);
    static uint16_t GetDynamicAffinityUtilThresh();
    static void SetDynamicAffinityUtilThresh(uint16_t value);
    static std::string GetSkippedCPUSet();
    static void SetSkippedCPUSet(const std::string &cpuSet);
    static bool IsRangeAffinity();
    static void SetRangeAffinity(bool value);

    static VasRet Init();
    static VasRet DeInit();
    static VasRet WriteDynamicAffinityUtilThresh(uint16_t value);
    static bool IsDynamicAffinityAvailable();

private:
    static std::mutex confMutex_;
};
} // namespace vas::sched

namespace vas::cli::reg {
cli::framework::VasCliSdkResult CliSetServerConfFunc(const std::map<std::string, std::string> &params);
void RegisterServerModuleSDK();
} // namespace vas::cli::reg

#endif // VAS_CLI_ARG_H
