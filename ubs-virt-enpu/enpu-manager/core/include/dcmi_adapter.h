/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef __DCMI_ADAPTER_H__
#define __DCMI_ADAPTER_H__

#include "common.h"
#include "dcmi_wrapper.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct dcmi_adapter dcmi_adapter_t;

typedef int (*dcmi_get_device_count_fn)(dcmi_adapter_t *adapter, int *count);
typedef int (*dcmi_get_device_list_fn)(dcmi_adapter_t *adapter, int *logic_ids, int max_count, int *actual_count);
typedef int (*dcmi_get_device_info_fn)(dcmi_adapter_t *adapter, int logic_id, dcmi_device_info_t *info);
typedef int (*dcmi_get_die_info_fn)(dcmi_adapter_t *adapter, int logic_id, dcmi_die_info_t *die_info);
typedef int (*dcmi_get_memory_info_fn)(dcmi_adapter_t *adapter, int logic_id, dcmi_memory_info_t *mem_info);
typedef int (*dcmi_get_chip_info_fn)(dcmi_adapter_t *adapter, int logic_id, uint32_t *aicore_num, uint32_t *hbm_size);

struct dcmi_adapter {
    dcmi_get_device_count_fn get_device_count;
    dcmi_get_device_list_fn get_device_list;
    dcmi_get_device_info_fn get_device_info;
    dcmi_get_die_info_fn get_die_info;
    dcmi_get_memory_info_fn get_memory_info;
    dcmi_get_chip_info_fn get_chip_info;
};

#if defined(__cplusplus)
}
#endif

#endif