/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 * ubs-virt-ovs is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include <stdarg.h>
#include "common.h"
#include "npu_manager.h"

static void die(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    int ret = vfprintf(stderr, fmt, ap);
    CHECK_COND_RETURN(ret < 0, "vfprintf failed.");
    va_end(ap);

    return;
}

static int parse_args(int argc, char *const argv[])
{
    if (argc > 1) {
        die("Invalid option : %s\n", argv[1]);
        return ENPU_FAIL;
    }
    return ENPU_SUCCESS;
}

static int monitor_npu_utilization(void)
{
    int ret;
    size_t used;

    ret = get_mem_used(&used);
    CHECK_RETURN_ERROR_CODE(ret, "Failed to get mem used.");

    die("       Aicore Limit Quota(%)     : %d\n"
        "       Memory Limit quota(MB)    : %lld\n"
        "       Memory Usage(MB)          : %d\n",
        get_core_limit_quota(), get_mem_limit_quota() / 1024 / 1024,  // 1024用于单位转换
        used / 1024 / 1024); // 1024用于单位转换

    return ENPU_SUCCESS;
}

int main(int argc, char *argv[])
{
    int ret;

    ret = log_init();
    CHECK_RETURN_ERROR_CODE(ret, "Log init failed.");

    ret = parse_args(argc, argv);
    CHECK_RETURN_ERROR_CODE(ret, "Failed to parse args.");

    if (getenv("ENPU_LOG_LEVEL") != NULL) {
        unsetenv("ENPU_LOG_LEVEL");
    }

    ret = enpu_load_config();
    CHECK_RETURN_ERROR_CODE(ret, "Load npu device failed.");

    ret = enpu_device_init();
    CHECK_RETURN_ERROR_CODE(ret, "Enpu device init failed.");

    ret = monitor_npu_utilization();
    CHECK_RETURN_ERROR_CODE(ret, "Npu utilization monitor failed.");

    return ENPU_SUCCESS;
}