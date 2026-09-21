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

#ifndef __DCMI_STUB_ADAPTER_H__
#define __DCMI_STUB_ADAPTER_H__

#include "common.h"
#include "dcmi_adapter.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct dcmi_stub_config {
    int device_count;
    int total_memory_mb;
    int total_aicore;
} dcmi_stub_config_t;

dcmi_adapter_t *dcmi_adapter_create_stub(const dcmi_stub_config_t *config);
void dcmi_adapter_destroy_stub(dcmi_adapter_t *adapter);

#if defined(__cplusplus)
}
#endif

#endif