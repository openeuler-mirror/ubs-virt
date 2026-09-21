/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 */

#ifndef __SWAP_MONITOR_THREAD_H__
#define __SWAP_MONITOR_THREAD_H__

#include <stdbool.h>
#include <stdint.h>
#include "common.h"
#include "memory_tracker.h"
#include "shm_manager.h"
#include "swap_executor.h"

struct swap_monitor_thread {
    int vnpu_id;
    shm_state_t *shm_state;
    swap_executor_t *executor;
    memory_tracker_t *tracker;
    uint64_t poll_interval_ms;
    pthread_t thread;
    atomic_bool running;
    atomic_bool stop_requested;
};

typedef struct swap_monitor_thread swap_monitor_thread_t;

#if defined(__cplusplus)
extern "C" {
#endif

swap_monitor_thread_t *swap_monitor_thread_create(int vnpu_id, shm_state_t *shm_state, void *executor, void *tracker,
                                                  uint64_t poll_interval_ms);
int swap_monitor_thread_destroy(swap_monitor_thread_t *thread);
swap_monitor_thread_t **swap_monitor_get_thread(void);
int swap_monitor_thread_start(swap_monitor_thread_t *thread);
int swap_monitor_thread_stop(swap_monitor_thread_t *thread);

#if defined(__cplusplus)
}
#endif

#endif