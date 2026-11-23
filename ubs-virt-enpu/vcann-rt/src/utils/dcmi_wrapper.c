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
#include "utils.h"

#include "dcmi_wrapper.h"

#define NPU_UTILIZATION (13)
#define NPU_AICORE_UTILIZATION (2)
#define AICORE_UTILIZATION (2)
#define MAX_DEVICE_LIST_NUM 64

typedef struct {
    int logic_id;
    int card_id;
    int device_id;
    uint8_t soc_version;
    uint64_t used_bytes;
    int result;
    atomic_uint references;
} mem_info_args;

static dcmi_operations g_dcmi_ops = {NULL};
static uint8_t g_dcmi_soc_version = SOC_VERSION_ASCEND_910;

static const enpu_dcmi_api_t *get_dcmi_api(const char **error)
{
#ifdef ENPU_DCMI_TEST_STUBS
    static const enpu_dcmi_api_t stub = {
        .dcmi_init = dcmi_init,
        .dcmi_get_device_utilization_rate = dcmi_get_device_utilization_rate,
        .dcmi_get_device_resource_info = dcmi_get_device_resource_info,
        .dcmi_get_card_id_device_id_from_phyid = dcmi_get_card_id_device_id_from_phyid,
        .dcmi_get_device_chip_info = dcmi_get_device_chip_info,
        .dcmiv2_init = dcmiv2_init,
        .dcmiv2_get_device_list = dcmiv2_get_device_list,
        .dcmiv2_get_device_utilization_rate = dcmiv2_get_device_utilization_rate,
        .dcmiv2_get_device_proc_mem_info = dcmiv2_get_device_proc_mem_info,
        .dcmiv2_get_device_chip_info = dcmiv2_get_device_chip_info,
    };
    (void)error;
    return &stub;
#else
    return enpu_dcmi_get_api(error);
#endif
}

int _dcmiv2_init_callback()
{
    return get_dcmi_api(NULL)->dcmiv2_init();
}

int _dcmi_init_callback()
{
    return get_dcmi_api(NULL)->dcmi_init();
}

int _dcmiv2_get_device_utilization_rate_callback(int logic_id, int card_id, int device_id, int input_type,
                                                 unsigned int *utilization)
{
    (void)card_id;
    (void)device_id;
    const enpu_dcmi_api_t *api = get_dcmi_api(NULL);
    return api->dcmiv2_get_device_utilization_rate != NULL ?
               api->dcmiv2_get_device_utilization_rate(logic_id, input_type, utilization) :
               ENPU_FAIL;
}

int _dcmi_get_device_utilization_rate_callback(int logic_id, int card_id, int device_id, int input_type,
                                               unsigned int *utilization)
{
    (void)logic_id;
    const enpu_dcmi_api_t *api = get_dcmi_api(NULL);
    return api->dcmi_get_device_utilization_rate != NULL ?
               api->dcmi_get_device_utilization_rate(card_id, device_id, input_type, utilization) :
               ENPU_FAIL;
}

int _dcmi_get_aicore_utilization_rate_callback(int logic_id, int card_id, int device_id, int input_type,
                                               unsigned int *utilization)
{
    (void)input_type;
    return _dcmi_get_device_utilization_rate_callback(logic_id, card_id, device_id, NPU_AICORE_UTILIZATION,
                                                      utilization);
}

int _dcmiv2_get_device_resource_info_callback(int logic_id, int card_id, int device_id,
                                              struct dcmi_proc_mem_info *proc_info, int *proc_num)
{
    (void)card_id;
    (void)device_id;
    const enpu_dcmi_api_t *api = get_dcmi_api(NULL);
    return api->dcmiv2_get_device_proc_mem_info != NULL ?
               api->dcmiv2_get_device_proc_mem_info(logic_id, proc_info, proc_num) :
               ENPU_FAIL;
}

int _dcmi_get_device_resource_info_callback(int logic_id, int card_id, int device_id,
                                            struct dcmi_proc_mem_info *proc_info, int *proc_num)
{
    (void)logic_id;
    const enpu_dcmi_api_t *api = get_dcmi_api(NULL);
    return api->dcmi_get_device_resource_info != NULL ?
               api->dcmi_get_device_resource_info(card_id, device_id, proc_info, proc_num) :
               ENPU_FAIL;
}

int _dcmiv2_get_device_aicore_num_callback(int logic_id, int card_id, int device_id, unsigned int *aicore_num)
{
    (void)card_id;
    (void)device_id;
    struct dcmi_chip_info_v2 chip_info;
    int ret = memset_s(&chip_info, sizeof(chip_info), 0, sizeof(chip_info));
    if (ret != 0) {
        LOG_ERROR("memset_s chip_info_v2 failed, ret=%d.", ret);
        return ENPU_FAIL;
    }
    ret = get_dcmi_api(NULL)->dcmiv2_get_device_chip_info(logic_id, &chip_info);
    if (ret != 0) {
        LOG_ERROR("dcmiv2_get_device_chip_info failed, logic_id=%d ret=%d.", logic_id, ret);
        return ret;
    }
    *aicore_num = chip_info.aicore_cnt;
    return 0;
}

int _dcmi_get_device_aicore_num_callback(int logic_id, int card_id, int device_id, unsigned int *aicore_num)
{
    (void)logic_id;
    struct dcmi_chip_info chip_info;
    int ret = memset_s(&chip_info, sizeof(chip_info), 0, sizeof(chip_info));
    if (ret != 0) {
        LOG_ERROR("memset_s chip_info failed, ret=%d.", ret);
        return ENPU_FAIL;
    }
    ret = get_dcmi_api(NULL)->dcmi_get_device_chip_info(card_id, device_id, &chip_info);
    if (ret != 0) {
        return ret;
    }
    *aicore_num = chip_info.aicore_cnt;
    return 0;
}

int register_callback(uint8_t soc_version)
{
    if (soc_version > SOC_VERSION_ASCEND_310) {
        LOG_ERROR("[DCMI] Unsupported SoC version %u", (unsigned int)soc_version);
        return ENPU_FAIL;
    }
    const char *error;
    const enpu_dcmi_api_t *api = get_dcmi_api(&error);
    if (api == NULL) {
        LOG_ERROR("[DCMI] Failed to load libdcmi.so: %s", error);
        return ENPU_FAIL;
    }
    if (soc_version == SOC_VERSION_ASCEND_950) {
        if (api->dcmiv2_init == NULL || api->dcmiv2_get_device_list == NULL ||
            api->dcmiv2_get_device_chip_info == NULL) {
            LOG_ERROR("[DCMI] Required Ascend 950 initialization, device list or chip info symbol is missing");
            return ENPU_FAIL;
        }
        // Ascend950 products need to call the series of dcmiv2 interface.
        g_dcmi_ops.init_callback = _dcmiv2_init_callback;
        g_dcmi_ops.get_device_utilization_rate_callback = _dcmiv2_get_device_utilization_rate_callback;
        g_dcmi_ops.get_device_resource_info_callback = _dcmiv2_get_device_resource_info_callback;
        g_dcmi_ops.get_device_aicore_num_callback = _dcmiv2_get_device_aicore_num_callback;
    } else {
        if (api->dcmi_init == NULL || api->dcmi_get_card_id_device_id_from_phyid == NULL ||
            api->dcmi_get_device_chip_info == NULL) {
            LOG_ERROR("[DCMI] Required initialization, physical ID mapping or chip info symbol is missing");
            return ENPU_FAIL;
        }
        g_dcmi_ops.init_callback = _dcmi_init_callback;
        if (soc_version == SOC_VERSION_ASCEND_310) {
            // Ascend310 products don't support get NPU total utilization, so get aicore utilization.
            g_dcmi_ops.get_device_utilization_rate_callback = _dcmi_get_aicore_utilization_rate_callback;
        } else {
            g_dcmi_ops.get_device_utilization_rate_callback = _dcmi_get_device_utilization_rate_callback;
        }
        g_dcmi_ops.get_device_resource_info_callback = _dcmi_get_device_resource_info_callback;
        g_dcmi_ops.get_device_aicore_num_callback = _dcmi_get_device_aicore_num_callback;
    }

    g_dcmi_soc_version = soc_version;
    return ENPU_SUCCESS;
}

int enpu_dcmi_get_card_info(uint32_t phy_id, int *card_id, int *device_id, int *logic_id, uint8_t soc_version)
{
    int ret;
    LOG_DEBUG("Enpu_dcmi_get_card_info begin");
    if (!card_id || !device_id || !logic_id) {
        LOG_ERROR("Invalid param");
        return ENPU_FAIL;
    }
    if (g_dcmi_ops.init_callback == NULL || soc_version != g_dcmi_soc_version) {
        LOG_ERROR("[DCMI] Callbacks are not registered for SoC version %u", (unsigned int)soc_version);
        return ENPU_FAIL;
    }

    ret = g_dcmi_ops.init_callback();
    CHECK_RETURN_ERROR_CODE(ret, "dcmi init failed.");
    LOG_DEBUG("dcmi_init success");

    if (soc_version == SOC_VERSION_ASCEND_910 || soc_version == SOC_VERSION_ASCEND_310) {
        ret = get_dcmi_api(NULL)->dcmi_get_card_id_device_id_from_phyid(card_id, device_id, phy_id);
        CHECK_RETURN_ERROR_CODE(ret, "get card info failed.");
        *logic_id = -1;
    } else {
        int device_list[MAX_DEVICE_LIST_NUM] = {0};
        int device_cnt = 0;

        ret = get_dcmi_api(NULL)->dcmiv2_get_device_list(device_list, &device_cnt, MAX_DEVICE_LIST_NUM);
        CHECK_RETURN_ERROR_CODE(ret, "get device list failed.");
        if (device_cnt <= 0 || device_cnt > MAX_DEVICE_LIST_NUM) {
            LOG_ERROR("[DCMI] Invalid v2 device count=%d", device_cnt);
            return ENPU_FAIL;
        }
        /*
         * DCMI v2 IDs are container-local and cannot be matched numerically
         * with the host physical-npu-id (for example, host phy 3 may appear
         * as local device 0). Each workload container exposes one NPU here,
         * so DCMI calls must use the first visible local device ID.
         */
        int selected = device_list[0];
        if (selected < 0) {
            LOG_ERROR("[DCMI] Invalid v2 logical device ID=%d", selected);
            return ENPU_FAIL;
        }
        *logic_id = selected;
        *card_id = -1;
        *device_id = selected;
    }
    LOG_DEBUG("enpu_dcmi_get_card_info success");
    return ENPU_SUCCESS;
}

int enpu_dcmi_get_utilization_rate(int logic_id, int card_id, int device_id, uint8_t soc_version, int input_type,
                                   unsigned int *utilization_rate)
{
    if (utilization_rate == NULL || soc_version > SOC_VERSION_ASCEND_310) {
        return ENPU_FAIL;
    }
    *utilization_rate = 0;
    if (input_type != ENPU_DCMI_UTIL_AICORE && input_type != ENPU_DCMI_UTIL_AICPU &&
        input_type != ENPU_DCMI_UTIL_CTRLCPU && input_type != ENPU_DCMI_UTIL_VECTOR &&
        input_type != ENPU_DCMI_UTIL_NPU && input_type != ENPU_DCMI_UTIL_CUBE) {
        return ENPU_FAIL;
    }
    const enpu_dcmi_api_t *api = get_dcmi_api(NULL);
    if (soc_version == SOC_VERSION_ASCEND_950) {
        if (api == NULL || api->dcmiv2_get_device_utilization_rate == NULL) {
            LOG_WARN("[VNPU-METRICS] DCMI v2 utilization symbol is unavailable, type=%d.", input_type);
            return ENPU_FAIL;
        }
        return _dcmiv2_get_device_utilization_rate_callback(logic_id, card_id, device_id, input_type, utilization_rate);
    }
    if (api == NULL || api->dcmi_get_device_utilization_rate == NULL) {
        LOG_WARN("[VNPU-METRICS] DCMI utilization symbol is unavailable, type=%d.", input_type);
        return ENPU_FAIL;
    }
    return _dcmi_get_device_utilization_rate_callback(logic_id, card_id, device_id, input_type, utilization_rate);
}

// Get NPU Utilization
int enpu_dcmi_get_device_utilization_rate(int logic_id, int card_id, int device_id, unsigned int *utilization_rate)
{
    if (utilization_rate == NULL) {
        LOG_ERROR("Invalid param: utilization_rate is NULL.");
        return ENPU_FAIL;
    }
    return g_dcmi_ops.get_device_utilization_rate_callback(logic_id, card_id, device_id, NPU_UTILIZATION,
                                                           utilization_rate);
}

// Get AI Core Utilization
int enpu_dcmi_get_aicore_utilization_rate(int logic_id, int card_id, int device_id, unsigned int *utilization_rate)
{
    if (utilization_rate == NULL) {
        LOG_ERROR("Invalid param: utilization_rate is NULL.");
        return ENPU_FAIL;
    }
    return g_dcmi_ops.get_device_utilization_rate_callback(logic_id, card_id, device_id, AICORE_UTILIZATION,
                                                           utilization_rate);
}

// Get the number of physical AI Cores of the device
int enpu_dcmi_get_aicore_num(int logic_id, int card_id, int device_id, unsigned int *aicore_num)
{
    if (aicore_num == NULL) {
        LOG_ERROR("enpu_dcmi_get_aicore_num: aicore_num is null.");
        return ENPU_FAIL;
    }
    return g_dcmi_ops.get_device_aicore_num_callback(logic_id, card_id, device_id, aicore_num);
}

int enpu_dcmi_query_hbm_usage(int logic_id, int card_id, int device_id, uint8_t soc_version, uint64_t *used_bytes)
{
    if (used_bytes == NULL || soc_version > SOC_VERSION_ASCEND_310) {
        return -EINVAL;
    }
    *used_bytes = 0;
    const enpu_dcmi_api_t *api = get_dcmi_api(NULL);
    if (api == NULL) {
        return -ENOSYS;
    }
    struct dcmi_proc_mem_info processes[MAX_PIDS];
    if (memset_s(processes, sizeof(processes), 0, sizeof(processes)) != 0) {
        return -EIO;
    }
    int count = MAX_PIDS;
    int ret;
    if (soc_version == SOC_VERSION_ASCEND_950) {
        if (logic_id < 0 || api->dcmiv2_get_device_proc_mem_info == NULL) {
            return -ENOSYS;
        }
        ret = api->dcmiv2_get_device_proc_mem_info(logic_id, processes, &count);
    } else {
        if (card_id < 0 || device_id < 0 || api->dcmi_get_device_resource_info == NULL) {
            return -ENOSYS;
        }
        ret = api->dcmi_get_device_resource_info(card_id, device_id, processes, &count);
    }
    if (ret != 0) {
        return ret;
    }
    /* Full capacity cannot prove that this fixed-size API returned a complete list. */
    if (count < 0 || count >= MAX_PIDS) {
        return -EMSGSIZE;
    }
    uint64_t sum = 0;
    for (int i = 0; i < count; ++i) {
        uint64_t bytes = (uint64_t)processes[i].proc_mem_usage;
        if (bytes > UINT64_MAX - sum) {
            return -EOVERFLOW;
        }
        sum += bytes;
    }
    *used_bytes = sum;
    return 0;
}

static void release_mem_info_args(mem_info_args *args)
{
    if (atomic_fetch_sub_explicit(&args->references, 1, memory_order_acq_rel) == 1) {
        free(args);
    }
}

static void *enpu_get_resource_info_thread(void *arg)
{
    mem_info_args *args = (mem_info_args *)arg;
    args->result =
        enpu_dcmi_query_hbm_usage(args->logic_id, args->card_id, args->device_id, args->soc_version, &args->used_bytes);
    release_mem_info_args(args);
    return NULL;
}

/* Compatibility entry for limiter/swap callers: retain the temporary-thread
 * behavior and generic failure result, but share the actual query/validation. */
int enpu_dcmi_get_device_resource_info(int logic_id, int card_id, int device_id, size_t *used)
{
    if (used == NULL) {
        LOG_ERROR("[DCMI] HBM output is NULL");
        return ENPU_FAIL;
    }
    *used = 0;
    if (g_dcmi_ops.get_device_resource_info_callback == NULL) {
        LOG_ERROR("[DCMI] HBM query callbacks are not registered");
        return ENPU_FAIL;
    }
    mem_info_args *args = calloc(1, sizeof(*args));
    if (args == NULL) {
        LOG_ERROR("[DCMI] Failed to allocate threaded HBM query context");
        return ENPU_FAIL;
    }
    args->logic_id = logic_id;
    args->card_id = card_id;
    args->device_id = device_id;
    args->soc_version = g_dcmi_soc_version;
    args->result = ENPU_FAIL;
    /* Caller and worker each own one reference. A join failure must not leave
     * a live worker accessing a returned caller's stack or freed context. */
    atomic_init(&args->references, 2);
    pthread_t thread;
    int ret = pthread_create(&thread, NULL, enpu_get_resource_info_thread, args);
    if (ret != 0) {
        LOG_ERROR("[DCMI] Failed to create HBM query thread: ret=%d", ret);
        free(args); /* No worker was started. */
        return ENPU_FAIL;
    }
    ret = pthread_join(thread, NULL);
    if (ret != 0) {
        LOG_ERROR("[DCMI] Failed to join HBM query thread: ret=%d", ret);
        (void)pthread_detach(thread);
        release_mem_info_args(args);
        return ENPU_FAIL;
    }
    int result = ENPU_SUCCESS;
    if (args->result != 0 || args->used_bytes > SIZE_MAX) {
        LOG_ERROR("[DCMI] HBM query failed: logic=%d card=%d device=%d ret=%d used_bytes=%llu", logic_id, card_id,
                  device_id, args->result, (unsigned long long)args->used_bytes);
        result = ENPU_FAIL;
    } else {
        *used = (size_t)args->used_bytes;
    }
    release_mem_info_args(args);
    return result;
}
