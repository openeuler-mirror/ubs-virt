/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 */

#include "swap_monitor_thread.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "log.h"
#include "npu_manager.h"
#include "shm_manager.h"

swap_monitor_thread_t *g_swap_monitor_thread = NULL;

swap_monitor_thread_t **swap_monitor_get_thread(void)
{
    return &g_swap_monitor_thread;
}

static void *swap_monitor_thread_func(void *arg)
{
    swap_monitor_thread_t *mon_thread = (swap_monitor_thread_t *)arg;
    int phy_id = atomic_load(&mon_thread->shm_state->phy_id);
    int ret = RUNTIME_HOOK_CALL(rt_library_entry, rtSetDevice, get_device_id());
    LOG_INFO("swap_monitor_thread_func: set device to phy_id=%d, vnpu_id=%d, ret=%d", phy_id, mon_thread->vnpu_id, ret);
    shm_set_swapped(mon_thread->shm_state, mon_thread->vnpu_id, false);
    while (atomic_load(&mon_thread->running)) {
        int32_t cmd_vnpu_id = 0;
        int32_t action = SWAP_ACTION_NONE;

        int rc = shm_check_swap_cmd(mon_thread->shm_state, &cmd_vnpu_id, &action);
        if (rc != 0) {
            usleep(mon_thread->poll_interval_ms * 1000);
            continue;
        }

        if (action == SWAP_ACTION_NONE) {
            usleep(mon_thread->poll_interval_ms * 1000);
            continue;
        }

        if (cmd_vnpu_id != mon_thread->vnpu_id) {
            usleep(mon_thread->poll_interval_ms * 1000);
            continue;
        }

        if (action == SWAP_ACTION_OUT) {
            bool already_swapped = atomic_load(&mon_thread->shm_state->entries[cmd_vnpu_id].swapped);
            if (already_swapped) {
                shm_ack_swap_cmd(mon_thread->shm_state);
                usleep(mon_thread->poll_interval_ms * 1000);
                continue;
            }
            LOG_INFO("swap_monitor_thread: detected swap_cmd action=%d for vnpu_id=%d, starting swap_out", action,
                     cmd_vnpu_id);

            vnpu_shm_entry_t *entry = shm_get_entry(mon_thread->shm_state, mon_thread->vnpu_id);
            if (entry == NULL) {
                LOG_ERROR("swap_monitor_thread: failed to get entry for vnpu_id=%d", mon_thread->vnpu_id);
                shm_ack_swap_cmd(mon_thread->shm_state);
                usleep(mon_thread->poll_interval_ms * 1000);
                continue;
            }

            pthread_mutex_lock(&mon_thread->tracker->lock);

            memory_record_t **records = NULL;
            int count = 0;

            ret = memory_tracker_collect_records(mon_thread->tracker, &records, &count);
            if (ret != ENPU_SUCCESS || count == 0) {
                LOG_ERROR("swap_monitor_thread: collect_records failed or no records, ret=%d, count=%d", ret, count);
                pthread_mutex_unlock(&mon_thread->tracker->lock);
                shm_ack_swap_cmd(mon_thread->shm_state);
                usleep(mon_thread->poll_interval_ms * 1000);
                continue;
            }
            uint64_t used = atomic_load(&entry->hbm_used);
            uint64_t act_swapped = 0;
            uint64_t swap_offset = entry->swap_offset;
            swap_executor_swap_out(mon_thread->executor, mon_thread->tracker, records, count, swap_offset,
                                   &act_swapped);
            free(records);

            uint64_t new_used = (act_swapped < used) ? (used - act_swapped) : 0;
            shm_update_used(mon_thread->shm_state, mon_thread->vnpu_id, new_used);
            shm_set_swapped(mon_thread->shm_state, mon_thread->vnpu_id, true);

            pthread_mutex_unlock(&mon_thread->tracker->lock);

            shm_ack_swap_cmd(mon_thread->shm_state);

            LOG_INFO("swap_monitor_thread: swap_out completed, count=%d, offset=%lu, updated hbm_used from %lu to %lu",
                     count, swap_offset, used, new_used);
        }

        usleep(mon_thread->poll_interval_ms * 1000);
    }

    LOG_INFO("swap_monitor_thread stopped for vnpu_id=%d", mon_thread->vnpu_id);
    return NULL;
}

swap_monitor_thread_t *swap_monitor_thread_create(int vnpu_id, shm_state_t *shm_state, void *executor, void *tracker,
                                                  uint64_t poll_interval_ms)
{
    if (shm_state == NULL || executor == NULL || tracker == NULL) {
        LOG_ERROR("swap_monitor_thread_create failed: shm_state=%p, executor=%p, tracker=%p", shm_state, executor,
                  tracker);
        return NULL;
    }

    if (vnpu_id < 0 || poll_interval_ms == 0) {
        LOG_ERROR("swap_monitor_thread_create failed: invalid vnpu_id=%d or poll_interval_ms=%lu", vnpu_id,
                  poll_interval_ms);
        return NULL;
    }

    swap_monitor_thread_t *thread = (swap_monitor_thread_t *)calloc(1, sizeof(swap_monitor_thread_t));
    if (thread == NULL) {
        LOG_ERROR("swap_monitor_thread_create failed: calloc returned NULL");
        return NULL;
    }

    thread->vnpu_id = vnpu_id;
    thread->shm_state = shm_state;
    thread->executor = (swap_executor_t *)executor;
    thread->tracker = (memory_tracker_t *)tracker;
    thread->poll_interval_ms = poll_interval_ms;
    atomic_store(&thread->running, false);
    atomic_store(&thread->stop_requested, false);

    return thread;
}

int swap_monitor_thread_destroy(swap_monitor_thread_t *thread)
{
    if (thread == NULL) {
        return ENPU_FAIL;
    }

    free(thread);
    return ENPU_SUCCESS;
}

int swap_monitor_thread_start(swap_monitor_thread_t *thread)
{
    if (thread == NULL) {
        LOG_ERROR("swap_monitor_thread_start failed: thread is NULL");
        return ENPU_FAIL;
    }

    atomic_store(&thread->running, true);
    atomic_store(&thread->stop_requested, false);

    int ret = pthread_create(&thread->thread, NULL, swap_monitor_thread_func, thread);
    if (ret != 0) {
        LOG_ERROR("swap_monitor_thread_start failed: pthread_create returned %d", ret);
        atomic_store(&thread->running, false);
        return ENPU_FAIL;
    }

    return ENPU_SUCCESS;
}

int swap_monitor_thread_stop(swap_monitor_thread_t *thread)
{
    if (thread == NULL) {
        return ENPU_FAIL;
    }

    atomic_store(&thread->running, false);

    pthread_join(thread->thread, NULL);

    return ENPU_SUCCESS;
}
