/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
* ubs-virt-ovs is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v2 for more details.
*/
#include "common.h"
#include "dcmi_wrapper.h"
#include "core_limiter.h"
#include "mem_limiter.h"
#include "utils.h"
#include "config.h"
#include "npu_manager.h"

pthread_once_t once_init = PTHREAD_ONCE_INIT;
static struct npu_manager g_npu_manager = {0};

bool is_mem_limit(void)
{
    return g_npu_manager.npu_info.is_mem_limit;
}

bool is_core_limit(void)
{
    return g_npu_manager.npu_info.is_core_limit;
}

size_t get_mem_limit_quota(void)
{
    return g_npu_manager.npu_info.mem_limit_quota;
}

uint8_t get_core_limit_quota(void)
{
    return g_npu_manager.npu_info.core_limit_quota;
}

uint8_t get_vnpu_id(void)
{
    return g_npu_manager.npu_info.vnpu_id;
}

char *get_vnpu_shm_id(void)
{
    return g_npu_manager.npu_info.shm_id;
}

uint64_t get_core_quota_timeslice(void)
{
    return g_npu_manager.npu_info.core_quota_timeslice;
}

void set_core_quota_timeslice(uint64_t time)
{
    g_npu_manager.npu_info.core_quota_timeslice = time;
}

uint64_t get_core_cur_timeslice(void)
{
    return g_npu_manager.npu_info.core_cur_timeslice;
}

void set_core_cur_timeslice(uint64_t time)
{
    g_npu_manager.npu_info.core_cur_timeslice = time;
}

schedule_policy_t get_sched_policy(void)
{
    return g_npu_manager.npu_info.sched_policy;
}

int get_mem_used(size_t *used)
{
    if (used == NULL) {
        LOG_ERROR("Failed to get memory usage, input argument used can not be NULL");
        return ENPU_FAIL;
    }

    npu_info *npu = &g_npu_manager.npu_info;
    int rc = enpu_dcmi_get_device_resource_info(npu->card_id, npu->device_id, used);
    CHECK_RETURN_ERROR_CODE(rc, "Failed to get device resource info.");
    return ENPU_SUCCESS;
}

static int enpu_config_info_init()
{
    CHECK_RETURN_RANGE_INT(config.phy_npu_id, 0, MAX_NPU_COUNT);
    CHECK_RETURN_RANGE_INT(config.vnpu_id, 0, MAX_VNPU);

    if (config.scheduling_policy == SCHED_POLICY_FIXED_SHARE ||
        config.scheduling_policy == SCHED_POLICY_ELASTIC) {
        CHECK_RETURN_RANGE_INT(config.aicore_quota, 1, MAX_CORE_QUOTA);
        CHECK_RETURN_RANGE_INT(config.memory_quota, 1, INT32_MAX);

        g_npu_manager.npu_info.core_limit_quota = (uint8_t)config.aicore_quota;
        g_npu_manager.npu_info.mem_limit_quota = (size_t)config.memory_quota * KB_TO_GB;
        g_npu_manager.npu_info.is_core_limit = true;
        g_npu_manager.npu_info.is_mem_limit = true;
    } else if (config.scheduling_policy == SCHED_POLICY_BEST_EFFORT) {
        g_npu_manager.npu_info.is_core_limit = false;
        g_npu_manager.npu_info.is_mem_limit = false;
    } else {
        LOG_ERROR("scheduling policy is illegal, %s = %d, should in range [0, %d]\n",
            OPTION_SCHEDULING_POLICY, config.scheduling_policy, SCHED_POLICY_BEST_EFFORT);
        return ENPU_FAIL;
    }

    g_npu_manager.npu_info.pnpu_id = config.phy_npu_id;
    g_npu_manager.npu_info.vnpu_id = config.vnpu_id;
    g_npu_manager.npu_info.sched_policy = config.scheduling_policy;

    if (strcpy_s(g_npu_manager.npu_info.shm_id, sizeof(g_npu_manager.npu_info.shm_id), config.shm_id) != 0) {
        LOG_ERROR("Failed to copy the shm_id from the config to the npu manager.");
        return ENPU_FAIL;
    }

    LOG_INFO("Successfully to initialize vnpu device.");
    return ENPU_SUCCESS;
}

int enpu_load_config(void)
{
    int rc = load_config(NPU_CONFIG_PATH);
    CHECK_RETURN_ERROR_CODE(rc, "Failed to initialize npu manager.");
    return enpu_config_info_init(config);
}

int enpu_device_init(void)
{
    int card_id = -1;
    int device_id = -1;
    int logic_id = g_npu_manager.npu_info.pnpu_id;
    int rc = enpu_dcmi_get_card_info(0, &card_id, &device_id);
    CHECK_RETURN_ERROR_CODE(rc, "Failed to get card info by enpu_device_init, err:%d npu:%d", rc, logic_id);

    g_npu_manager.npu_info.card_id = card_id;
    g_npu_manager.npu_info.device_id = device_id;
    return ENPU_SUCCESS;
}

static void __enpu_global_init(void)
{
    int rc = log_init();
    CHECK_ERROR_CODE(rc, "Failed to init log module.");

    rc = enpu_load_config();
    CHECK_ERROR_CODE(rc, "Failed to load npu config.");

    rc = enpu_device_init();
    CHECK_ERROR_CODE(rc, "Failed to initialize enpu device.");

    rc = memory_limiter_init();
    CHECK_ERROR_CODE(rc, "Failed to initialize memory limiter");

    rc = aicore_limiter_initialize();
    CHECK_ERROR_CODE(rc, "Failed to initialize aicore limiter");

    LOG_INFO("Successfully to initialize all module.");
}

void enpu_global_init(void)
{
    pthread_once(&once_init, __enpu_global_init);
}