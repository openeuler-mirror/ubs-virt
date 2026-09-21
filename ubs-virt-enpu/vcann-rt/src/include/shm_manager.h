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
#ifndef __SHM_MANAGER_H__
#define __SHM_MANAGER_H__

#include "common.h"

#if defined(__cplusplus)
#include <atomic>
using atomic_uint_fast64_t = std::atomic<uint_fast64_t>;
using atomic_bool = std::atomic<bool>;
using atomic_int = std::atomic<int>;
#else
#include <stdatomic.h>
#endif

#define SHM_STATE_VERSION 2
#define SWAP_ACTION_NONE 0
#define SWAP_ACTION_OUT 1
#define SWAP_ACTION_IN 2
#define SWAP_OUT_NONE 0
#define SWAP_OUT_FROM_BORROWED 1
#define SWAP_OUT_FROM_ALL 2

#define MAX_UUID_LEN 64
#define DIE_ID_LEN 64
#define MAX_VNPU_PER_DIE 128
#define MB_TO_B (1024 * 1024)

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct swap_cmd {
    char target_pod_uid[MAX_UUID_LEN];
    int32_t target_vnpu_id;
    atomic_int action;
    atomic_bool completed;
    uint64_t timestamp_ns;
} swap_cmd_t;

typedef struct swap_out_cmd {
    int32_t target_vnpu_id;
    atomic_int action;
    atomic_bool completed;
} swap_out_cmd_t;

typedef struct vnpu_shm_entry {
    int32_t vnpu_id;
    uint64_t hbm_request;
    uint64_t hbm_limit;
    atomic_uint_fast64_t hbm_used;
    int32_t sched_policy;
    bool swapped;
    uint64_t swap_offset;
    uint64_t swap_size;
    bool swap_enabled;
    int32_t swap_priority;
} vnpu_shm_entry_t;

typedef struct shm_state {
    uint32_t version;
    int32_t phy_id;
    char shm_id[DIE_ID_LEN];
    uint64_t hbm_total;
    vnpu_shm_entry_t entries[MAX_VNPU_PER_DIE];
    atomic_uint_fast64_t vnpu_bitmap[2];
    int32_t sched_policy;
    atomic_bool initialized;
    void *swap_buffer_base;
    swap_cmd_t swap_cmd;
    swap_out_cmd_t swap_out_cmd;
    pthread_mutex_t swap_in_mutex;
    uint8_t reserved[64];
} shm_state_t;

shm_state_t *shm_state_posix_shm_attach(int phy_id, const char *die_id);
void shm_state_posix_shm_detach(shm_state_t *state);

vnpu_shm_entry_t *shm_get_entry(shm_state_t *state, int32_t vnpu_id);
int shm_update_used(shm_state_t *state, int32_t vnpu_id, uint64_t used);
uint64_t shm_get_dynamic_free(shm_state_t *state);
uint64_t shm_get_hbm_request_free(shm_state_t *state);
uint64_t shm_get_swap_size(shm_state_t *state, int32_t vnpu_id);
int shm_update_hbm_request(shm_state_t *state, int32_t vnpu_id, uint64_t request_quota);

int shm_check_swap_cmd(shm_state_t *state, int32_t *vnpu_id, int32_t *action);
void shm_ack_swap_cmd(shm_state_t *state);
void shm_set_swapped(shm_state_t *state, int32_t vnpu_id, bool swapped);
bool shm_get_swapped(shm_state_t *state, int32_t vnpu_id);

static inline bool vnpu_bitmap_test(const atomic_uint_fast64_t bitmap[2], int vnpu_id)
{
    if (vnpu_id < 0 || vnpu_id >= MAX_VNPU_PER_DIE)
        return false;
    int idx = vnpu_id / 64;
    int bit = vnpu_id % 64;
    return (atomic_load(&bitmap[idx]) >> bit) & 1;
}

static inline void vnpu_bitmap_set(atomic_uint_fast64_t bitmap[2], int vnpu_id)
{
    if (vnpu_id < 0 || vnpu_id >= MAX_VNPU_PER_DIE)
        return;
    int idx = vnpu_id / 64;
    int bit = vnpu_id % 64;
    atomic_fetch_or(&bitmap[idx], (1ULL << bit));
}

#if defined(__cplusplus)
}
#endif

#endif