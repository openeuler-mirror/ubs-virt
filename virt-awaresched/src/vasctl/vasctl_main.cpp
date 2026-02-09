/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#include "error.h"
#include "vasctl_arg_parse.h"
#include "vas_cli_parse.h"
#include "vas_cli_process_ctl.h"

using namespace vas::cli::reg;
using namespace vas::cli::framework;
using namespace vas::common;
using namespace vas::cli;
constexpr int MAX_CLI_ARGS = 10;
int main(int argc, char *argv[])
{
    if (argc > MAX_CLI_ARGS) {
        VasCliParse::PrintWithWordWrap("INFO: Unsupported number of parameters");
        return static_cast<int>(VAS_ERROR);
    }
    if (argv == nullptr) {
        VasCliParse::PrintWithWordWrap("INFO: System input error.");
        return static_cast<int>(VAS_ERROR);
    }
    RegisterCliModuleSDK();
    const auto ret = VasCliProcessCtl::MainExecuteProcess(argc, argv);
    return static_cast<int>(ret);
}