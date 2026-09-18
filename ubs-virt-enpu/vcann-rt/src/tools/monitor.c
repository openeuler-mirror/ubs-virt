/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* RTLD_NODELETE */
#endif
#include <dlfcn.h>
#include <stdarg.h>
#include "common.h"
#include "dcmi_wrapper.h"
#include "npu_manager.h"
#include "runtime_hook.h"
#include "vnpu_stats.h"

static void die(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    int ret = vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (ret < 0) {
        LOG_ERROR("vfprintf failed.");
    }

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

static int load_rt_for_monitor(void)
{
    void *handle = dlopen("libruntime.so", RTLD_LAZY | RTLD_NODELETE);
    if (!handle) {
        LOG_ERROR("Failed to dlopen libruntime.so: %s", dlerror());
        return ENPU_FAIL;
    }

    for (int i = 0; i < RUNTIME_ENTRY_END; i++) {
        rt_library_entry[i].func_ptr = dlsym(handle, rt_library_entry[i].name);
        if (rt_library_entry[i].func_ptr == NULL) {
            LOG_DEBUG("Monitor: function %s not found, skipped.", rt_library_entry[i].name);
        }
    }
    dlclose(handle);
    return ENPU_SUCCESS;
}

static int monitor_npu_utilization(void)
{
    int ret;
    size_t used;

    ret = get_mem_used(&used);
    CHECK_RETURN_ERROR_CODE(ret, "Failed to get mem used.");

    die("       Aicore Limit Quota(%)     : %d\n"
        "       Memory Limit quota(MB)    : %zu\n"
        "       Memory Usage(MB)          : %zu\n",
        get_core_limit_quota(), get_mem_limit_quota() / 1024 / 1024, // 1024用于单位转换
        used / 1024 / 1024);                                         // 1024用于单位转换

    return ENPU_SUCCESS;
}

static int monitor_vnpu_utilization(void)
{
    int ret = vnpu_stats_init(get_vnpu_shm_id());
    if (ret != ENPU_SUCCESS) {
        LOG_WARN("vNPU stats shared memory unavailable, skip vNPU Utilization output.");
        return ENPU_SUCCESS;
    }

    unsigned int npu_util = 0;
    ret = enpu_dcmi_get_device_utilization_rate(get_logic_id(), get_card_id(), get_device_id(), &npu_util);
    if (ret != ENPU_SUCCESS) {
        LOG_WARN("Failed to get physical NPU utilization (ret=%d), skip vNPU Utilization output.", ret);
        return ENPU_SUCCESS;
    }

    unsigned int aicore_util = 0;
    ret = enpu_dcmi_get_aicore_utilization_rate(get_logic_id(), get_card_id(), get_device_id(), &aicore_util);
    if (ret != ENPU_SUCCESS) {
        LOG_WARN("Failed to get AI Core utilization (ret=%d), skip vNPU Utilization output.", ret);
        return ENPU_SUCCESS;
    }

    uint8_t my_vnpu = get_vnpu_id();
    double numerator = 0.0;
    double denominator = 0.0;
    for (int i = 0; i < MAX_VNPU; ++i) {
        vnpu_stats_aggregate_t agg = {0};
        if (vnpu_stats_query((uint8_t)i, &agg) != ENPU_SUCCESS) {
            continue;
        }
        if (agg.sum_block_dim == 0ULL) {
            continue;
        }
        int64_t avg_duration = vnpu_stats_get_avg_duration_ns((uint8_t)i);
        double term = (double)agg.sum_block_dim * (double)avg_duration;
        denominator += term;
        if ((uint8_t)i == my_vnpu) {
            numerator = term;
        }
    }

    double weight = (denominator > 0.0) ? (numerator / denominator) : 0.0;
    double vnpu_util = 0.0;
    double vnpu_aicore_util = 0.0;
    if (denominator > 0.0) {
        vnpu_util = weight * (double)npu_util;
        vnpu_aicore_util = weight * (double)aicore_util;
        if (vnpu_util < 0.0) {
            vnpu_util = 0.0;
        }
        if (vnpu_aicore_util < 0.0) {
            vnpu_aicore_util = 0.0;
        }
    }

    die("       NPU Utilization(%)        : %.1f\n"
        "       vNPU Utilization(%)       : %.1f\n"
        "       NPU AICore Usage Rate(%)  : %.1f\n"
        "       vNPU AICore Usage Rate(%) : %.1f\n",
        (double)npu_util, vnpu_util, (double)aicore_util, vnpu_aicore_util);
    return ENPU_SUCCESS;
}

int main(int argc, char *argv[])
{
    int ret;

    /* enpu-monitor 只包含业务输出（配额/使用率）*/
    g_log_silent = true;

    ret = log_init();
    CHECK_RETURN_ERROR_CODE(ret, "Log init failed.");

    ret = parse_args(argc, argv);
    CHECK_RETURN_ERROR_CODE(ret, "Failed to parse args.");

    if (getenv("ENPU_LOG_LEVEL") != NULL) {
        unsetenv("ENPU_LOG_LEVEL");
    }

    ret = load_rt_for_monitor();
    CHECK_RETURN_ERROR_CODE(ret, "Failed to load runtime library for monitor.");

    ret = enpu_soc_init();
    CHECK_RETURN_ERROR_CODE(ret, "Enpu soc init failed.");

    ret = enpu_load_config();
    CHECK_RETURN_ERROR_CODE(ret, "Load npu device failed.");

    ret = enpu_device_init();
    CHECK_RETURN_ERROR_CODE(ret, "Enpu device init failed.");

    ret = monitor_npu_utilization();
    CHECK_RETURN_ERROR_CODE(ret, "Npu utilization monitor failed.");

    ret = monitor_vnpu_utilization();
    CHECK_RETURN_ERROR_CODE(ret, "vNPU utilization monitor failed.");

    return ENPU_SUCCESS;
}