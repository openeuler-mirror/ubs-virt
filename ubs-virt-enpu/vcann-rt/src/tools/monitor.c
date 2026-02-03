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
    if (ret < 0) {
        LOG_ERROR("vfprintf failed");
        return;
    }
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
    if (ret != ENPU_SUCCESS) {
        return ENPU_FAIL;
    }

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

    ret = parse_args(argc, argv);
    if (ret != ENPU_SUCCESS) {
        return ENPU_FAIL;
    }

    ret = log_init();
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("log init failed");
        return ENPU_FAIL;
    }

    if (getenv("ENPU_LOG_LEVEL") != NULL) {
        unsetenv("ENPU_LOG_LEVEL");
    }

    ret = enpu_load_config();
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("load npu device failed");
        return ENPU_FAIL;
    }

    ret = enpu_device_init();
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("enpu_device_init failed");
        return ENPU_FAIL;
    }

    ret = monitor_npu_utilization();
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("npu utilization monitor failed");
        return ENPU_FAIL;
    }

    return ENPU_SUCCESS;
}