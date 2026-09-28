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

#include "../../../../cli/parser/test_vas_cli_parse.h"

#include <string>
#include <vector>

#include "vas_cli_parse.h"
#include "vas_cli_reg_builder.h"

using namespace vas::cli::framework;

namespace vas::ut::cli {
TEST_F(TestVasCliParse, testPrintWithLineLimitSmallPositiveStepProgresses)
{
    auto &parser = VasCliParse::GetInstance();
    testing::internal::CaptureStdout();
    parser.PrintWithLineLimit(std::string(120, 'x'), 64, "\n");
    const std::string output = testing::internal::GetCapturedStdout();
    EXPECT_FALSE(output.empty());
}

TEST_F(TestVasCliParse, testSetSdkCmdInfoWithOptsMapRejectsInvalidOptions)
{
    auto &parser = VasCliParse::GetInstance();
    std::vector<VasCliSdkCmdInfo> sdkRegInfo;
    sdkRegInfo.push_back({"duplicate-short", "type", "", {{"x", "long-a", ""}, {"x", "long-b", ""}}, nullptr});
    VasCliParse::VasCliRegisterSdkCmdInfo(sdkRegInfo);
    EXPECT_TRUE(parser.GetSdkCmdInfo().empty());

    parser.Reset();
    sdkRegInfo.clear();
    sdkRegInfo.push_back({"duplicate-long", "type", "", {{"x", "long-a", ""}, {"y", "long-a", ""}}, nullptr});
    VasCliParse::VasCliRegisterSdkCmdInfo(sdkRegInfo);
    EXPECT_TRUE(parser.GetSdkCmdInfo().empty());

    parser.Reset();
    std::vector<VasCliSdkOptionsInfo> tooManyOptions;
    for (int i = 0; i <= MAX_OPTIONS_NUM; ++i) {
        const std::string suffix = std::to_string(i);
        tooManyOptions.push_back({"s" + suffix, "long" + suffix, ""});
    }
    sdkRegInfo.clear();
    sdkRegInfo.push_back({"too-many-options", "type", "", tooManyOptions, nullptr});
    VasCliParse::VasCliRegisterSdkCmdInfo(sdkRegInfo);
    EXPECT_TRUE(parser.GetSdkCmdInfo().empty());
}
} // namespace vas::ut::cli
