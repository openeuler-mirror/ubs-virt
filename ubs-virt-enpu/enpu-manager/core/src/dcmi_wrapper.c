/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 */

#include <securec.h>

#include <acl/acl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dcmi_interface_api.h"
#include "dcmi_wrapper.h"
#include "log.h"

/* dcmi_get_device_utilization_rate 的input_type取值 */
#define NPU_UTILIZATION_TYPE_TOTAL (13)
#define NPU_UTILIZATION_TYPE_AICORE (2)
#define NPU_UTILIZATION_TYPE_AICPU (3)

static pthread_mutex_t g_dcmi_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_dcmi_initialized = 0;
static soc_version_t g_soc_version = SOC_VERSION_NOT_ASCEND_950;

soc_version_t dcmi_wrapper_get_soc_version(void)
{
    return g_soc_version;
}

static void detect_soc_version(void)
{
    const char *soc_name = aclrtGetSocName();
    if (soc_name != NULL && strstr(soc_name, "Ascend950") != NULL) {
        g_soc_version = SOC_VERSION_ASCEND_950;
    } else {
        g_soc_version = SOC_VERSION_NOT_ASCEND_950;
    }
    LOG_DEBUG("[DCMI] Detected SoC: name=%s, soc_version=%d (0=A5, 1=A2A3)", soc_name ? soc_name : "(null)",
              g_soc_version);
}

#define CHECK_PARAM_NULL(param, ret)                               \
    do {                                                           \
        if ((param) == NULL) {                                     \
            LOG_ERROR("[DCMI] Invalid param: %s is NULL", #param); \
            return (ret);                                          \
        }                                                          \
    } while (0)

#define CHECK_INIT()                                          \
    do {                                                      \
        if (g_dcmi_initialized == 0) {                        \
            LOG_ERROR("[DCMI] DCMI wrapper not initialized"); \
            return ENPU_FAIL;                                 \
        }                                                     \
    } while (0)

int dcmi_wrapper_init(void)
{
    pthread_mutex_lock(&g_dcmi_mutex);
    if (g_dcmi_initialized) {
        pthread_mutex_unlock(&g_dcmi_mutex);
        LOG_DEBUG("[DCMI] DCMI wrapper already initialized");
        return ENPU_SUCCESS;
    }

    detect_soc_version();

    int ret;
    if (g_soc_version == SOC_VERSION_ASCEND_950) {
        ret = dcmiv2_init();
    } else {
        ret = dcmi_init();
    }
    if (ret != 0) {
        pthread_mutex_unlock(&g_dcmi_mutex);
        LOG_ERROR("[DCMI] Failed to init libdcmi.so: %s returned %d",
                  g_soc_version == SOC_VERSION_ASCEND_950 ? "dcmiv2_init" : "dcmi_init", ret);
        return ENPU_FAIL;
    }

    g_dcmi_initialized = 1;

    pthread_mutex_unlock(&g_dcmi_mutex);
    LOG_DEBUG("[DCMI] DCMI wrapper initialized successfully (soc_version=%d)", g_soc_version);
    return ENPU_SUCCESS;
}

void dcmi_wrapper_fini(void)
{
    pthread_mutex_lock(&g_dcmi_mutex);
    g_dcmi_initialized = 0;
    pthread_mutex_unlock(&g_dcmi_mutex);
    LOG_DEBUG("[DCMI] DCMI wrapper finalized");
}

int dcmi_get_device_count(int *count)
{
    CHECK_PARAM_NULL(count, ENPU_INVALID_PARAM);
    CHECK_INIT();

    int ret;
    if (g_soc_version == SOC_VERSION_ASCEND_950) {
        ret = dcmiv2_get_all_device_count(count);
    } else {
        ret = dcmi_get_all_device_count(count);
    }
    if (ret != 0) {
        LOG_ERROR("[DCMI] get_all_device_count failed: %d (soc_version=%d)", ret, g_soc_version);
        return ENPU_FAIL;
    }

    LOG_DEBUG("[DCMI] dcmi_get_device_count: count=%d", *count);
    return ENPU_SUCCESS;
}

int dcmi_get_device_list(int *logic_ids, int max_count, int *actual_count)
{
    CHECK_PARAM_NULL(logic_ids, ENPU_INVALID_PARAM);
    CHECK_PARAM_NULL(actual_count, ENPU_INVALID_PARAM);
    CHECK_INIT();

    if (max_count <= 0) {
        LOG_ERROR("[DCMI] Invalid max_count: %d", max_count);
        return ENPU_INVALID_PARAM;
    }

    if (g_soc_version == SOC_VERSION_ASCEND_950) {
        int ret = dcmiv2_get_device_list(logic_ids, actual_count, max_count);
        if (ret != 0) {
            LOG_ERROR("[DCMI] dcmiv2_get_device_list failed: %d", ret);
            return ENPU_FAIL;
        }
        LOG_DEBUG("[DCMI] dcmi_get_device_list(A5): actual_count=%d", *actual_count);
        return ENPU_SUCCESS;
    }

    int total = 0;
    int ret = dcmi_get_all_device_count(&total);
    if (ret != 0) {
        LOG_ERROR("[DCMI] dcmi_get_all_device_count failed: %d", ret);
        return ENPU_FAIL;
    }

    int copy_count = (total < max_count) ? total : max_count;
    for (int i = 0; i < copy_count; i++) {
        logic_ids[i] = i;
    }
    *actual_count = copy_count;

    LOG_DEBUG("[DCMI] dcmi_get_device_list(A2A3): actual_count=%d (total=%d)", *actual_count, total);
    return ENPU_SUCCESS;
}

int dcmi_get_device_info(int logic_id, dcmi_device_info_t *info)
{
    CHECK_PARAM_NULL(info, ENPU_INVALID_PARAM);
    CHECK_INIT();

    (void)memset_s(info, sizeof(dcmi_device_info_t), 0, sizeof(dcmi_device_info_t));
    info->logic_id = logic_id;

    if (g_soc_version == SOC_VERSION_ASCEND_950) {
        /* Ascend950: logic_id == device_id */
        info->device_id = logic_id;
        info->card_id = DCMI_INVALID_CARD_ID;

        struct dcmi_hbm_info hbm = {0};
        int ret = dcmiv2_get_device_hbm_info(logic_id, &hbm);
        if (ret != 0) {
            LOG_ERROR("[DCMI] dcmiv2_get_device_hbm_info(dev_id=%d) failed: %d", logic_id, ret);
            return ENPU_FAIL;
        }
        info->total_memory = hbm.memory_size;

        struct dcmi_chip_info_v2 chip = {0};
        ret = dcmiv2_get_device_chip_info(logic_id, &chip);
        if (ret != 0) {
            LOG_DEBUG("[DCMI] dcmiv2_get_device_chip_info failed=%d", ret);
        }
        info->total_aicore = HUNDRED_CORE;

        ret = snprintf_s(info->uuid, MAX_UUID_LEN, MAX_UUID_LEN - 1, "uuid-%d", logic_id);
        if (ret < 0) {
            LOG_ERROR("[DCMI] snprintf_s(uuid) failed: ret=%d", ret);
            return ENPU_FAIL;
        }
        return ENPU_SUCCESS;
    }

    int ret = dcmi_get_card_id_device_id_from_logicid(&info->card_id, &info->device_id, (unsigned int)logic_id);
    if (ret != 0) {
        LOG_ERROR("[DCMI] logic_id=%d -> card/device failed: %d", logic_id, ret);
        return ENPU_FAIL;
    }

    struct dcmi_hbm_info hbm = {0};
    ret = dcmi_get_device_hbm_info(info->card_id, info->device_id, &hbm);
    if (ret != 0) {
        LOG_ERROR("[DCMI] dcmi_get_device_hbm_info(card=%d, device=%d) failed: %d", info->card_id, info->device_id,
                  ret);
        return ENPU_FAIL;
    }
    info->total_memory = hbm.memory_size;

    struct dcmi_chip_info_v2 chip = {0};
    ret = dcmi_get_device_chip_info_v2(info->card_id, info->device_id, &chip);
    if (ret != 0) {
        LOG_DEBUG("[DCMI] dcmi_get_device_chip_info_v2 failed=%d", ret);
    }
    info->total_aicore = HUNDRED_CORE;

    ret = snprintf_s(info->uuid, MAX_UUID_LEN, MAX_UUID_LEN - 1, "uuid-%d-%d", info->card_id, info->device_id);
    if (ret < 0) {
        LOG_ERROR("[DCMI] snprintf_s(uuid) failed: ret=%d", ret);
        return ENPU_FAIL;
    }
    return ENPU_SUCCESS;
}

int dcmi_get_card_info(int logic_id, int *card_id, int *device_id)
{
    CHECK_PARAM_NULL(card_id, ENPU_INVALID_PARAM);
    CHECK_PARAM_NULL(device_id, ENPU_INVALID_PARAM);
    CHECK_INIT();

    if (g_soc_version == SOC_VERSION_ASCEND_950) {
        /* Ascend950: logic_id == device_id */
        *card_id = DCMI_INVALID_CARD_ID;
        *device_id = logic_id;
        return ENPU_SUCCESS;
    }

    int ret = dcmi_get_card_id_device_id_from_logicid(card_id, device_id, (unsigned int)logic_id);
    if (ret != 0) {
        LOG_ERROR("[DCMI] dcmi_get_card_id_device_id_from_logicid(logic_id=%d) failed: %d", logic_id, ret);
        return ENPU_FAIL;
    }
    return ENPU_SUCCESS;
}

int dcmi_get_logic_id(int card_id, int device_id, int *logic_id)
{
    CHECK_PARAM_NULL(logic_id, ENPU_INVALID_PARAM);
    CHECK_INIT();

    if (g_soc_version == SOC_VERSION_ASCEND_950) {
        /* Ascend950: logic_id == device_id */
        *logic_id = device_id;
        return ENPU_SUCCESS;
    }

    int ret = dcmi_get_device_logic_id(logic_id, card_id, device_id);
    if (ret != 0) {
        LOG_ERROR("[DCMI] dcmi_get_device_logic_id(card=%d, device=%d) failed: %d", card_id, device_id, ret);
        return ENPU_FAIL;
    }
    return ENPU_SUCCESS;
}

int dcmi_get_die_info(int logic_id, dcmi_die_info_t *die_info)
{
    CHECK_PARAM_NULL(die_info, ENPU_INVALID_PARAM);
    CHECK_INIT();

    die_info->phy_id = logic_id;

    if (g_soc_version == SOC_VERSION_ASCEND_950) {
        die_info->card_id = DCMI_INVALID_CARD_ID;
        die_info->device_id = logic_id;

        struct dcmi_die_id did = {0};
        int ret = dcmiv2_get_device_die_id(logic_id, DDIE, &did);
        if (ret != 0) {
            LOG_ERROR("[DCMI] dcmiv2_get_device_die_id(dev=%d, DDIE) failed: %d", logic_id, ret);
            return ENPU_FAIL;
        }
        ret = snprintf_s(die_info->die_id, DIE_ID_LEN, DIE_ID_LEN - 1, "%08X-%08X-%08X-%08X-%08X", did.soc_die[0],
                         did.soc_die[1], did.soc_die[2], did.soc_die[3], did.soc_die[4]);
        if (ret < 0) {
            LOG_ERROR("[DCMI] snprintf_s(die_id) failed: ret=%d", ret);
            return ENPU_FAIL;
        }
        ret = snprintf_s(die_info->shm_id, DIE_ID_LEN, DIE_ID_LEN - 1, "%s", die_info->die_id);
        if (ret < 0) {
            LOG_ERROR("[DCMI] snprintf_s(shm_id) failed: ret=%d", ret);
            return ENPU_FAIL;
        }
        LOG_DEBUG("[DCMI] dcmi_get_die_info(A5): die_id=%s", die_info->die_id);
        return ENPU_SUCCESS;
    }

    int card_id = 0, device_id = 0;
    int ret = dcmi_get_card_id_device_id_from_logicid(&card_id, &device_id, (unsigned int)logic_id);
    if (ret != 0) {
        LOG_ERROR("[DCMI] logic_id=%d -> card/device failed: %d", logic_id, ret);
        return ENPU_FAIL;
    }

    struct dcmi_die_id did = {0};
    ret = dcmi_get_device_die_v2(card_id, device_id, VDIE, &did);
    if (ret != 0) {
        LOG_ERROR("[DCMI] dcmi_get_device_die_v2(card=%d, device=%d, VDIE) failed: %d", card_id, device_id, ret);
        return ENPU_FAIL;
    }

    die_info->card_id = card_id;
    die_info->device_id = device_id;
    ret = snprintf_s(die_info->die_id, DIE_ID_LEN, DIE_ID_LEN - 1, "%08X-%08X-%08X-%08X-%08X", did.soc_die[0],
                     did.soc_die[1], did.soc_die[2], did.soc_die[3], did.soc_die[4]);
    if (ret < 0) {
        LOG_ERROR("[DCMI] snprintf_s(die_id) failed: ret=%d", ret);
        return ENPU_FAIL;
    }
    ret = snprintf_s(die_info->shm_id, DIE_ID_LEN, DIE_ID_LEN - 1, "%s", die_info->die_id);
    if (ret < 0) {
        LOG_ERROR("[DCMI] snprintf_s(shm_id) failed: ret=%d", ret);
        return ENPU_FAIL;
    }

    return ENPU_SUCCESS;
}

int dcmi_get_shm_id_by_logic_id(int logic_id, char *shm_id, size_t shm_id_size)
{
    CHECK_PARAM_NULL(shm_id, ENPU_INVALID_PARAM);
    CHECK_INIT();

    dcmi_die_info_t die_info = {0};
    int ret = dcmi_get_die_info(logic_id, &die_info);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }

    ret = snprintf_s(shm_id, shm_id_size, shm_id_size - 1, "%s", die_info.die_id);
    if (ret < 0) {
        LOG_ERROR("[DCMI] snprintf_s(shm_id) failed: ret=%d", ret);
        return ENPU_FAIL;
    }
    return ENPU_SUCCESS;
}

int dcmi_get_utilization(int logic_id, dcmi_utilization_t *utilization)
{
    CHECK_PARAM_NULL(utilization, ENPU_INVALID_PARAM);
    CHECK_INIT();

    int card_id = 0, device_id = 0;
    int ret = dcmi_get_card_id_device_id_from_logicid(&card_id, &device_id, (unsigned int)logic_id);
    if (ret != 0) {
        LOG_ERROR("[DCMI] logic_id=%d -> card/device failed: %d", logic_id, ret);
        return ENPU_FAIL;
    }
    return dcmi_get_utilization_by_card(card_id, device_id, utilization);
}

int dcmi_get_utilization_by_card(int card_id, int device_id, dcmi_utilization_t *utilization)
{
    CHECK_PARAM_NULL(utilization, ENPU_INVALID_PARAM);
    CHECK_INIT();

    int ret = 0;
    if (g_soc_version == SOC_VERSION_ASCEND_950) {
        int dev_id = device_id;
        ret |=
            dcmiv2_get_device_utilization_rate(dev_id, NPU_UTILIZATION_TYPE_AICORE, &utilization->aicore_utilization);
        ret |= dcmiv2_get_device_utilization_rate(dev_id, NPU_UTILIZATION_TYPE_AICPU, &utilization->aicpu_utilization);
        ret |= dcmiv2_get_device_utilization_rate(dev_id, NPU_UTILIZATION_TYPE_TOTAL, &utilization->total_utilization);
    } else {
        ret |= dcmi_get_device_utilization_rate(card_id, device_id, NPU_UTILIZATION_TYPE_AICORE,
                                                &utilization->aicore_utilization);
        ret |= dcmi_get_device_utilization_rate(card_id, device_id, NPU_UTILIZATION_TYPE_AICPU,
                                                &utilization->aicpu_utilization);
        ret |= dcmi_get_device_utilization_rate(card_id, device_id, NPU_UTILIZATION_TYPE_TOTAL,
                                                &utilization->total_utilization);
    }

    if (ret != 0) {
        LOG_ERROR("[DCMI] get_device_utilization_rate(card=%d, device=%d, soc=%d) failed: %d", card_id, device_id,
                  g_soc_version, ret);
        return ENPU_FAIL;
    }
    return ENPU_SUCCESS;
}

int dcmi_get_memory_info(int logic_id, dcmi_memory_info_t *mem_info)
{
    CHECK_PARAM_NULL(mem_info, ENPU_INVALID_PARAM);
    CHECK_INIT();

    int card_id = 0, device_id = 0;
    int ret = dcmi_get_card_id_device_id_from_logicid(&card_id, &device_id, (unsigned int)logic_id);
    if (ret != 0) {
        LOG_ERROR("[DCMI] logic_id=%d -> card/device failed: %d", logic_id, ret);
        return ENPU_FAIL;
    }
    return dcmi_get_memory_info_by_card(card_id, device_id, mem_info);
}

int dcmi_get_memory_info_by_card(int card_id, int device_id, dcmi_memory_info_t *mem_info)
{
    CHECK_PARAM_NULL(mem_info, ENPU_INVALID_PARAM);
    CHECK_INIT();

    int ret = dcmi_get_process_memory(card_id, device_id, &mem_info->used_memory);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }

    struct dcmi_hbm_info hbm = {0};
    if (g_soc_version == SOC_VERSION_ASCEND_950) {
        ret = dcmiv2_get_device_hbm_info(device_id, &hbm);
    } else {
        ret = dcmi_get_device_hbm_info(card_id, device_id, &hbm);
    }

    if (ret != 0) {
        LOG_ERROR("[DCMI] get_device_hbm_info(card=%d, device=%d, soc=%d) failed: %d", card_id, device_id,
                  g_soc_version, ret);
        return ENPU_FAIL;
    }
    mem_info->total_memory = hbm.memory_size;
    mem_info->free_memory =
        (mem_info->total_memory > mem_info->used_memory) ? (mem_info->total_memory - mem_info->used_memory) : 0;
    return ENPU_SUCCESS;
}

int dcmi_get_process_memory(int card_id, int device_id, uint64_t *used)
{
    CHECK_PARAM_NULL(used, ENPU_INVALID_PARAM);
    CHECK_INIT();

    struct dcmi_proc_mem_info proc_info[DCMI_MAX_PIDS];
    int proc_num = DCMI_MAX_PIDS;
    (void)memset_s(proc_info, sizeof(proc_info), 0, sizeof(proc_info));

    int ret;
    if (g_soc_version == SOC_VERSION_ASCEND_950) {
        ret = dcmiv2_get_device_proc_mem_info(device_id, proc_info, &proc_num);
    } else {
        ret = dcmi_get_device_resource_info(card_id, device_id, proc_info, &proc_num);
    }

    if (ret != 0) {
        LOG_ERROR("[DCMI] get_device_resource_info(card=%d, device=%d, soc=%d) failed: %d", card_id, device_id,
                  g_soc_version, ret);
        return ENPU_FAIL;
    }

    if (proc_num > DCMI_MAX_PIDS) {
        LOG_ERROR("[DCMI] too many processes: %d (max %d)", proc_num, DCMI_MAX_PIDS);
        proc_num = DCMI_MAX_PIDS;
    }

    uint64_t total_used = 0;
    for (int i = 0; i < proc_num; i++) {
        total_used += proc_info[i].proc_mem_usage;
    }
    *used = total_used;

    LOG_DEBUG("[DCMI] dcmi_get_process_memory: card=%d device=%d proc_num=%d used=%lu", card_id, device_id, proc_num,
              (unsigned long)*used);
    return ENPU_SUCCESS;
}