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

#include <dcmi_interface_api.h>
#include "utils.h"

#include "dcmi_wrapper.h"

typedef struct {
    int card_id;
    int device_id;
    struct dcmi_proc_mem_info *proc_info;
    int *proc_num;
    int result;
} mem_info_args;

int enpu_dcmi_get_card_info(int logic_id, int *card_id, int *device_id)
{
    int ret;
    LOG_DEBUG("Enpu_dcmi_get_card_info begin");
    if (!card_id || !device_id) {
        LOG_ERROR("Invalid param");
        return ENPU_FAIL;
    }

    ret = dcmi_init();
    CHECK_RETURN_ERROR_CODE(ret, "dcmi init failed.");
    LOG_DEBUG("dcmi_init success");

    ret = dcmi_get_card_id_device_id_from_logicid(card_id, device_id, logic_id);
    CHECK_RETURN_ERROR_CODE(ret, "get card info failed.");
    LOG_DEBUG("enpu_dcmi_get_card_info success");
    return ENPU_SUCCESS;
}

static void *enpu_get_resource_info_thread(void *arg)
{
    mem_info_args *args = (mem_info_args *)arg;
    int proc_num_temp = 0;

    args->result = dcmi_get_device_resource_info(args->card_id, args->device_id, args->proc_info, &proc_num_temp);

    if (args->proc_num != NULL) {
        *(args->proc_num) = proc_num_temp;
    }
    return NULL;
}

static int enpu_get_mem_used(struct dcmi_proc_mem_info *proc_info, int proc_num, size_t *used)
{
    if (proc_info == NULL || used == NULL) {
        LOG_ERROR("Invalid parameters");
        return ENPU_FAIL;
    }

    if (proc_num > MAX_PIDS) {
        LOG_ERROR("dcmi get device resource too many processes, count is %d", proc_num);
        return DCMI_ERR_CODE_INNER_ERR;
    }

    *used = 0;

    for (int i = 0; i < proc_num; i++) {
        *used += proc_info[i].proc_mem_usage;
        LOG_DEBUG("Contain PID %d, mem usage:%zd", proc_info[i].proc_id, proc_info[i].proc_mem_usage);
    }
    LOG_DEBUG("dcmi get mem used as %zd", *used);
    return ENPU_SUCCESS;
}


int enpu_dcmi_get_device_resource_info(int card_id, int device_id, size_t *used)
{
    int proc_num = 0;
    struct dcmi_proc_mem_info proc_info[MAX_PIDS];

    if (used == NULL) {
        LOG_ERROR("Invalid parameters");
        return ENPU_FAIL;
    }

    memset_s(proc_info, sizeof(proc_info), 0, sizeof(proc_info));
    mem_info_args args = { .card_id = card_id,
        .device_id = device_id,
        .proc_info = proc_info,
        .proc_num = &proc_num,
        .result = ENPU_FAIL};
    
    pthread_t thread;

    int ret = pthread_create(&thread, NULL, enpu_get_resource_info_thread, &args);
    CHECK_COND_RETURN_ERROR_CODE(ret != 0, "create thread failed.");

    ret = pthread_join(thread, NULL);
    CHECK_COND_RETURN_ERROR_CODE(ret != 0, "join thread failed.");

    if (args.result != 0) {
        LOG_ERROR("get info failed card:%d device:%d result:%d", card_id, device_id, args.result);
        return ENPU_FAIL;
    }

    ret = enpu_get_mem_used(proc_info, proc_num, used);
    CHECK_RETURN_ERROR_CODE(ret, "get device mem info failed card-id:%d device_id:%d.",
        card_id, device_id);

    return ENPU_SUCCESS;
}