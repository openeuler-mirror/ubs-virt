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
#ifndef __NPU_MANAGER_H__
#define __NPU_MANAGER_H__

#if defined(__cplusplus)
#include <atomic>
using atomic_int = std::atomic<int>;
using atomic_uint_fast8_t = std::atomic<uint_fast8_t>;
using atomic_uint_fast32_t = std::atomic<uint_fast32_t>;
using atomic_uint_fast64_t = std::atomic<uint_fast64_t>;
#else
#include <stdatomic.h>
#endif

#include "config.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define NPU_CONFIG_PATH "/etc/enpu/vcann-rt/npu_info.config"
#define MAX_NPU_ID 15
#define MAX_VNPU 100
#define MAX_CORE_QUOTA 100
#define MB_TO_B (1024 * 1024)
#define MAX_DEVICE_LIST_NUM 64

typedef enum
{
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
    atomic_int slide_window_len;
    atomic_uint_fast64_t last_slide_window_time_ns;
} vnpu_time_slice_sched_t;

typedef struct npu_info {
    uint32_t pnpu_id;
    int logic_id;
    int card_id;
    int device_id;
    uint8_t vnpu_id;
    bool in_used;
    size_t mem_limit_quota;
    uint8_t core_limit_quota;
    uint64_t core_quota_timeslice;
    int64_t core_cur_timeslice;
    bool is_core_limit;
    schedule_policy_t sched_policy;
    char shm_id[SHM_ID_LEN];
    bool initialization;
    uint8_t soc_version;
} npu_info;

extern void enpu_global_init(void);
extern void enpu_global_init_post(void);

extern bool is_core_limit(void);
extern uint8_t get_core_limit_quota(void);
extern size_t get_mem_limit_quota(void);
extern void set_mem_limit_quota(size_t mem);
extern char *get_vnpu_shm_id(void);
extern int get_mem_used(size_t *used);
extern int get_device_id(void);
extern uint8_t get_vnpu_id(void);
extern uint64_t get_core_quota_timeslice(void);
extern void set_core_quota_timeslice(uint64_t time);
extern int64_t get_core_cur_timeslice(void);
extern void set_core_cur_timeslice(int64_t time);
extern int get_card_id(void);
extern schedule_policy_t get_sched_policy(void);
extern bool check_init_success(void);
extern uint8_t get_soc_version(void);
extern int get_logic_id(void);

extern int enpu_load_config(void);
extern int enpu_device_init(void);
extern int enpu_config_info_init(void);
extern int enpu_soc_init(void);
#if defined(__cplusplus)
}
#endif

#endif