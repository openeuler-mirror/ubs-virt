/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 *
 * VSched is licensed under the Mulan PSL v2.
 * You may obtain a copy of the License at:
 *      http://license.coscl.org.cn/MulanPSL2
 */
#ifndef VASCTL_TEST_EXPORTS_H
#define VASCTL_TEST_EXPORTS_H

#include <map>
#include <string>

#include "vas_cli_reg_builder.h"

namespace vas::cli::reg {
framework::VasCliSdkResult CliSetConfFunc(const std::map<std::string, std::string> &params);
framework::VasCliSdkResult CliQueryAffinityFunc(const std::map<std::string, std::string> &params);
framework::VasCliSdkResult CliOptReassignFunc(const std::map<std::string, std::string> &params);
framework::VasCliSdkResult CliOptRecoverFunc(const std::map<std::string, std::string> &params);
} // namespace vas::cli::reg

#endif // VASCTL_TEST_EXPORTS_H
