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

#ifndef __DCMI_WRAPPER_H__
#define __DCMI_WRAPPER_H__

#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define MAX_PIDS 128

extern int enpu_dcmi_get_card_info(int logic_id, int *card_id, int *device_id);
extern int enpu_dcmi_get_device_resource_info(int card_id, int device_id, size_t *used);

#if defined(__cplusplus)
}
#endif

#endif