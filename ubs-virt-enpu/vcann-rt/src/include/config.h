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
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "common.h"

#define SHM_ID_LEN 128
#define OPTION_NPU_ID "physical-npu-id"
#define OPTION_VNPU_ID "virtual-npu-id"
#define OPTION_AICORE_QUOTA "aicore-quota"
#define OPTION_MEMORY_QUOTA "memory-quota"
#define OPTION_SHM_ID "shm-id"
#define OPTION_SCHEDULING_POLICY "scheduling-policy"
#define INVALID_VALUE (-1)

struct Config {
    int32_t phy_npu_id;
    int32_t vnpu_id;
    int32_t aicore_quota;
    int32_t memory_quota;
    int32_t scheduling_policy;
    char shm_id[SHM_ID_LEN];
};

extern struct Config config;

int load_config(const char *file_path);

#endif