/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 */

#include "pod_watchdog.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../internal/npu_allocator.h"
#include "log.h"
#include "shm_manager.h"
#include "swap_buffer_manager.h"

struct pod_watchdog {
    shm_manager_t *shm_mgr;
    swap_buffer_manager_t *swap_buf_mgr;
    npu_allocator_t *allocator;
};

static uint64_t wd_now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

pod_watchdog_t *pod_watchdog_create(void *shm_mgr, void *swap_buf_mgr, void *allocator)
{
    if (shm_mgr == NULL || swap_buf_mgr == NULL) {
        return NULL;
    }

    pod_watchdog_t *wd = (pod_watchdog_t *)calloc(1, sizeof(pod_watchdog_t));
    if (wd == NULL) {
        return NULL;
    }

    wd->shm_mgr = (shm_manager_t *)shm_mgr;
    wd->swap_buf_mgr = (swap_buffer_manager_t *)swap_buf_mgr;
    wd->allocator = (npu_allocator_t *)allocator;

    return wd;
}

int pod_watchdog_destroy(pod_watchdog_t *wd)
{
    if (wd == NULL) {
        return ENPU_FAIL;
    }

    free(wd);
    return ENPU_SUCCESS;
}

int pod_watchdog_cleanup_swap_data(pod_watchdog_t *wd, int phy_id, int vnpu_id)
{
    if (wd == NULL) {
        return ENPU_FAIL;
    }

    bool swapped = false;
    uint64_t offset = 0, size = 0;

    int ret = shm_manager_get_swap_state(wd->shm_mgr, phy_id, vnpu_id, &swapped, &offset, &size);
    if (ret == ENPU_NOT_FOUND) {
        return ENPU_NOT_FOUND;
    }
    if (ret != ENPU_SUCCESS) {
        return ret;
    }

    if (!swapped) {
        return ENPU_SUCCESS;
    }

    shm_state_t *state = shm_manager_get_state(wd->shm_mgr, phy_id);
    if (state != NULL) {
        shm_free(state, vnpu_id);
    }

    ret = shm_manager_set_swap_state(wd->shm_mgr, phy_id, vnpu_id, 0, 0);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }

    return ENPU_SUCCESS;
}

static void reset_runtime_state(shm_state_t *state, vnpu_time_slice_sched_t *sched, int vnpu_id)
{
    vnpu_shm_entry_t *entry = &state->entries[vnpu_id];
    atomic_store(&entry->hbm_used, 0);
    entry->swapped = false;
    entry->swap_offset = 0;
    entry->swap_size = 0;

    if (sched != NULL) {
        atomic_store(&sched->last_alive_time_ns[vnpu_id], 0);
    }
}

int pod_watchdog_poll_and_cleanup(pod_watchdog_t *wd)
{
    if (wd == NULL) {
        return ENPU_FAIL;
    }

    uint64_t now_ns = wd_now_ns();

    for (int phy_id = 0; phy_id < MAX_NPU_PER_NODE; phy_id++) {
        shm_state_t *state = wd->shm_mgr->shm_states[phy_id];
        if (state == NULL) {
            continue;
        }

        vnpu_time_slice_sched_t *sched = shm_manager_get_sched_struct(wd->shm_mgr, phy_id);
        if (sched == NULL) {
            LOG_WARN("[POD-WATCHDOG] phy_id=%d sched_struct missing, skip all vNPUs on this DIE", phy_id);
            continue;
        }

        uint64_t bitmap0 = atomic_load(&state->vnpu_bitmap[0]);
        uint64_t bitmap1 = atomic_load(&state->vnpu_bitmap[1]);

        for (int vnpu_id = 0; vnpu_id < MAX_VNPU_PER_DIE; vnpu_id++) {
            int idx = vnpu_id / 64;
            int bit = vnpu_id % 64;
            uint64_t bitmap = (idx == 0) ? bitmap0 : bitmap1;

            if (!((bitmap >> bit) & 1)) {
                continue;
            }

            uint64_t last_alive = atomic_load(&sched->last_alive_time_ns[vnpu_id]);
            if (last_alive == 0) {
                continue; /* vcann-rt never started for this vnpu, not dead */
            }

            /* uint64 下溢保护：系统重启后 CLOCK_MONOTONIC 归零，但 shm 保留上次的
             * last_alive（比当前 now_ns 大），相减会下溢到接近 uint64_max。
             * 这种情况是"时钟重置"而非"vcann-rt 死亡"，清零 last_alive 并跳过。 */
            if (last_alive > now_ns) {
                LOG_WARN("[POD-WATCHDOG] phy_id=%d vnpu_id=%d last_alive > now "
                         "(clock reset after reboot?), recalibrating to 0",
                         phy_id, vnpu_id);
                atomic_store(&sched->last_alive_time_ns[vnpu_id], 0);
                continue;
            }

            if (now_ns - last_alive <= POD_ALIVE_TIMEOUT_NS) {
                continue; /* still alive */
            }

            /* 清理超时的 vNPU 的 swap 资源和运行时资源 */
            vnpu_shm_entry_t *entry = &state->entries[vnpu_id];
            bool was_swapped = entry->swapped;

            LOG_WARN("[POD-WATCHDOG] phy_id=%d vnpu_id=%d timeout (last_alive %.3fs ago > %.3fs); "
                     "cleaning runtime state (was_swapped=%d)",
                     phy_id, vnpu_id, (now_ns - last_alive) / 1000000000.0, POD_ALIVE_TIMEOUT_NS / 1000000000.0,
                     was_swapped);

            if (was_swapped) {
                int ret = pod_watchdog_cleanup_swap_data(wd, phy_id, vnpu_id);
                if (ret != ENPU_SUCCESS && ret != ENPU_NOT_FOUND) {
                    LOG_ERROR("[POD-WATCHDOG] cleanup_swap_data failed: phy_id=%d vnpu_id=%d ret=%d", phy_id, vnpu_id,
                              ret);
                }
            }

            reset_runtime_state(state, sched, vnpu_id);
        }
    }

    return ENPU_SUCCESS;
}