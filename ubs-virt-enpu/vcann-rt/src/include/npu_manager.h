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
#ifndef __NPU_MANAGER_H__
#define __NPU_MANAGER_H__

#include <stdatomic.h>
#include "config.h"

#define NPU_CONFIG_PATH  "/etc/enpu/npu_info.config"
#define MAX_PIDS 1024
#define MAX_NPU_COUNT 16
#define MAX_VNPU 100
#define MAX_CORE_QUOTA 100
#define MB_TO_B (1024 * 1024)

typedef enum {
    SCHED_POLICY_FIXED_SHARE = 1,
    SCHED_POLICY_ELASTIC = 2,
    SCHED_POLICY_BEST_EFFORT = 3,
} schedule_policy_t;

typedef struct shared_memory {
    pthread_mutex_t vnpu_schedule_mutex[MAX_VNPU];
    atomic_uint_fast64_t last_alive_time_ns[MAX_VNPU];
    atomic_uint_fast64_t last_kernel_time_ns[MAX_VNPU];
    atomic_uint_fast8_t vnpu_schedule_turn[MAX_VNPU];
    atomic_uint_fast8_t vnpu_core_limit_quota[MAX_VNPU];
    atomic_int owner;
    atomic_uint_fast32_t magic_number;
} vnpu_time_slice_sched_t;

typedef struct npu_info {
    uint8_t pnpu_id;
    uint8_t logic_id;
    uint8_t card_id;
    uint8_t device_id;
    uint8_t vnpu_id;
    bool in_used;
    size_t mem_limit_quota;
    uint8_t core_limit_quota;
    uint64_t core_quota_timeslice;
    uint64_t core_cur_timeslice;
    bool is_mem_limit;
    bool is_core_limit;
    schedule_policy_t sched_policy;
    char shm_id[SHM_ID_LEN];
} npu_info;

typedef struct npu_manager {
    npu_info npu_info;
} npu_manager;

extern void enpu_global_init(void);

extern bool is_core_limit(void);
extern bool is_mem_limit(void);
extern uint8_t get_core_limit_quota(void);
extern size_t get_mem_limit_quota(void);
extern char *get_vnpu_shm_id(void);
extern int get_mem_used(size_t *used);
extern uint8_t get_vnpu_id(void);
extern uint64_t get_core_quota_timeslice(void);
extern void set_core_quota_timeslice(uint64_t time);
extern uint64_t get_core_cur_timeslice(void);
extern void set_core_cur_timeslice(uint64_t time);
extern schedule_policy_t get_sched_policy(void);

extern int enpu_load_config(void);
extern int enpu_device_init(void);

#endif