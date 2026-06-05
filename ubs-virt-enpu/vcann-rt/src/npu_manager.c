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

#include "npu_manager.h"
#include <runtime/rt.h>
#include "acl/acl.h"
#include "common.h"
#include "config.h"
#include "core_limiter.h"
#include "dcmi_wrapper.h"
#include "include/common.h"
#include "mem_limiter.h"
#include "runtime_hook.h"
#include "utils.h"

pthread_once_t once_init = PTHREAD_ONCE_INIT;
pthread_once_t post_init_flag = PTHREAD_ONCE_INIT;
static struct npu_info g_npu_info = {0};

bool is_core_limit(void)
{
    return g_npu_info.is_core_limit;
}

size_t get_mem_limit_quota(void)
{
    return g_npu_info.mem_limit_quota;
}

void set_mem_limit_quota(size_t mem)
{
    g_npu_info.mem_limit_quota = mem;
}

uint8_t get_core_limit_quota(void)
{
    return g_npu_info.core_limit_quota;
}

int get_device_id(void)
{
    return g_npu_info.device_id;
}

int get_logic_id(void)
{
    return g_npu_info.logic_id;
}

uint8_t get_soc_version(void)
{
    return g_npu_info.soc_version;
}

uint8_t get_vnpu_id(void)
{
    return g_npu_info.vnpu_id;
}

char *get_vnpu_shm_id(void)
{
    return g_npu_info.shm_id;
}

uint64_t get_core_quota_timeslice(void)
{
    return g_npu_info.core_quota_timeslice;
}

void set_core_quota_timeslice(uint64_t time)
{
    g_npu_info.core_quota_timeslice = time;
}

int64_t get_core_cur_timeslice(void)
{
    return g_npu_info.core_cur_timeslice;
}

void set_core_cur_timeslice(int64_t time)
{
    g_npu_info.core_cur_timeslice = time;
}

int get_card_id(void)
{
    return g_npu_info.card_id;
}

schedule_policy_t get_sched_policy(void)
{
    return g_npu_info.sched_policy;
}

bool check_init_success(void)
{
    return g_npu_info.initialization;
}

int get_mem_used(size_t *used)
{
    if (used == NULL) {
        LOG_ERROR("Failed to get memory usage, input argument used can not be NULL");
        return ENPU_FAIL;
    }

    npu_info *npu = &g_npu_info;
    int rc = enpu_dcmi_get_device_resource_info(npu->logic_id, npu->card_id, npu->device_id, used);
    CHECK_RETURN_ERROR_CODE(rc, "Failed to get device resource info.");
    return ENPU_SUCCESS;
}

int enpu_config_info_init()
{
    CHECK_RETURN_RANGE_INT(config.phy_npu_id, 0, MAX_NPU_ID);
    CHECK_RETURN_RANGE_INT(config.vnpu_id, 0, MAX_VNPU);

    size_t max_memory_quota = SIZE_MAX / MB_TO_B;
    CHECK_RETURN_RANGE_INT((size_t)config.memory_quota, 1, max_memory_quota);
    if (config.scheduling_policy == SCHED_POLICY_FIXED_SHARE || config.scheduling_policy == SCHED_POLICY_ELASTIC) {
        CHECK_RETURN_RANGE_INT(config.aicore_quota, 1, MAX_CORE_QUOTA);

        g_npu_info.core_limit_quota = (uint8_t)config.aicore_quota;
        g_npu_info.mem_limit_quota = (size_t)config.memory_quota * MB_TO_B;
        g_npu_info.is_core_limit = true;
    } else if (config.scheduling_policy == SCHED_POLICY_BEST_EFFORT) {
        g_npu_info.mem_limit_quota = (size_t)config.memory_quota * MB_TO_B;
        g_npu_info.is_core_limit = false;
    } else {
        LOG_ERROR("scheduling policy is illegal, %s = %d, should in range [0, %d]\n", OPTION_SCHEDULING_POLICY,
                  config.scheduling_policy, SCHED_POLICY_BEST_EFFORT);
        return ENPU_FAIL;
    }

    g_npu_info.pnpu_id = config.phy_npu_id;
    g_npu_info.vnpu_id = config.vnpu_id;
    g_npu_info.sched_policy = config.scheduling_policy;

    int ret = strcpy_s(g_npu_info.shm_id, sizeof(g_npu_info.shm_id), config.shm_id);
    CHECK_COND_RETURN_ERROR_CODE(ret != 0, "Failed to copy the shm_id from the config to the npu manager.");

    LOG_INFO("Successfully to initialize vnpu device.");
    return ENPU_SUCCESS;
}

int enpu_load_config(void)
{
    int rc = load_config(NPU_CONFIG_PATH);
    CHECK_RETURN_ERROR_CODE(rc, "Failed to initialize npu manager.");
    return enpu_config_info_init();
}

int enpu_soc_init(void)
{
    const char *socName = aclrtGetSocName();
    CHECK_COND_RETURN_ERROR_CODE(socName == NULL, "Call aclrtGetSocName fails.");
    LOG_INFO("Get socName: %s.", socName);

    if (strstr(socName, "Ascend950") != NULL) {
        g_npu_info.soc_version = SOC_VERSION_ASCEND_950;
    } else {
        g_npu_info.soc_version = SOC_VERSION_NOT_ASCEND_950;
    }

    int ret = register_callback(g_npu_info.soc_version);
    CHECK_RETURN_ERROR_CODE(ret, "Failed to register callback.");

    return ENPU_SUCCESS;
}

int enpu_device_init(void)
{
    int card_id = -1;
    int device_id = -1;
    int logic_id = -1;

    int rc = enpu_dcmi_get_card_info(g_npu_info.pnpu_id, &card_id, &device_id, &logic_id, g_npu_info.soc_version);
    CHECK_RETURN_ERROR_CODE(rc, "Failed to get card info by enpu_device_init, err:%d npu:%d", rc, g_npu_info.pnpu_id);

    g_npu_info.card_id = card_id;
    g_npu_info.device_id = device_id;
    g_npu_info.logic_id = logic_id;
    return ENPU_SUCCESS;
}

static void __enpu_global_init(void)
{
    int rc = log_init();
    CHECK_COND_RETURN_LOG(rc != ENPU_SUCCESS, "Failed to init log module.");

    rc = enpu_load_config();
    CHECK_COND_RETURN(rc != ENPU_SUCCESS, "Failed to load npu config.");

    rc = enpu_soc_init();
    CHECK_COND_RETURN(rc != ENPU_SUCCESS, "Failed to initialize enpu soc.");

    rc = enpu_device_init();
    CHECK_COND_RETURN(rc != ENPU_SUCCESS, "Failed to initialize enpu device.");

    rc = memory_limiter_init();
    CHECK_COND_RETURN(rc != ENPU_SUCCESS, "Failed to initialize memory limiter");

    rc = aicore_limiter_initialize();
    CHECK_COND_RETURN(rc != ENPU_SUCCESS, "Failed to initialize aicore limiter");

    rc = setenv("ENPU_ENABLE", "True", 1);
    CHECK_COND_RETURN(rc != ENPU_SUCCESS, "Failed to set environment variable");

    g_npu_info.initialization = true;
    LOG_INFO("Successfully to initialize all module.");
}

void enpu_global_init(void)
{
    pthread_once(&once_init, __enpu_global_init);
}

void __enpu_global_init_post(void)
{
    size_t freeSize = 0;
    size_t totalSize = 0;
    size_t appliedSize = get_mem_limit_quota();
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtMemGetInfoEx, RT_MEMORYINFO_HBM, &freeSize, &totalSize);
    LOG_DEBUG("Call rtMemGetInfoEx return:%d, free HBM size:%zu, total HBM size:%zu, user applied HBM size:%zu.", ret,
              freeSize, totalSize, appliedSize);
    CHECK_COND_LOG_((ret != RT_ERROR_NONE), "Get avaliable HBM size failed! ret:%d, freeSize:%zu, totalSize:%zu.", ret,
                    freeSize, totalSize);
    if (appliedSize > totalSize) {
        LOG_WARN("User appiled HBM size:%zd is bigger than total HBM size:%zd, now set mem_limit_quota to %zd.",
                 appliedSize, totalSize, totalSize);
        set_mem_limit_quota(totalSize);
    }
}

void enpu_global_init_post(void)
{
    pthread_once(&post_init_flag, __enpu_global_init_post);
}
