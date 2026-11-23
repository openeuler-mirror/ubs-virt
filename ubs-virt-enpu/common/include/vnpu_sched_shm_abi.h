/* Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 * Licensed under Mulan PSL v2.
 */
#ifndef ENPU_VNPU_SCHED_SHM_ABI_H
#define ENPU_VNPU_SCHED_SHM_ABI_H
#include <pthread.h>
#include <stdint.h>
#ifdef __cplusplus
#include <atomic>
using atomic_int = std::atomic<int>;
using atomic_uint_fast8_t = std::atomic<uint_fast8_t>;
using atomic_uint_fast32_t = std::atomic<uint_fast32_t>;
using atomic_uint_fast64_t = std::atomic<uint_fast64_t>;
#else
#include <stdatomic.h>
#endif

/* Both manager and runtime must use the same scheduling ABI. */
#define VNPU_SCHED_MAX_VNPU 100
#define VNPU_SCHED_MAGIC_INITIALIZED 0x495A4548U /* v4: shared launch budget added */
#define VNPU_SCHED_MAGIC_INITIALIZING 0x5A494E47U
#define VNPU_SCHED_MAGIC_UNINITIALIZED 0U

enum
{
    VNPU_LAUNCH_IDLE,
    VNPU_LAUNCH_OPEN,
    VNPU_LAUNCH_DRAIN,
};

typedef struct {
    atomic_uint_fast64_t launch_count;
    atomic_uint_fast64_t launch_budget;
    atomic_uint_fast64_t duration_ns;
    atomic_uint_fast64_t alpha_per_mille;
    atomic_uint_fast64_t model_fallbacks;
    atomic_uint_fast64_t epoch;
    atomic_uint_fast64_t turn_id;
    atomic_uint_fast64_t start_ns;
    atomic_uint_fast64_t deadline_ns;
    atomic_uint_fast64_t stop_ns;
    atomic_uint_fast64_t drain_launches;
    atomic_int enabled;
    atomic_int yield_requested;
    atomic_int duration_fallback;
    atomic_int phase;
    atomic_int failed;
} vnpu_launch_budget_t;

static inline void vnpu_launch_budget_init(vnpu_launch_budget_t *budget)
{
    atomic_store(&budget->launch_count, 0);
    atomic_store(&budget->launch_budget, 0);
    atomic_store(&budget->duration_ns, 0);
    atomic_store(&budget->alpha_per_mille, 1000);
    atomic_store(&budget->model_fallbacks, 0);
    atomic_store(&budget->epoch, 0);
    atomic_store(&budget->turn_id, 0);
    atomic_store(&budget->start_ns, 0);
    atomic_store(&budget->deadline_ns, 0);
    atomic_store(&budget->stop_ns, 0);
    atomic_store(&budget->drain_launches, 0);
    atomic_store(&budget->enabled, 0);
    atomic_store(&budget->yield_requested, 0);
    atomic_store(&budget->duration_fallback, 0);
    atomic_store(&budget->phase, VNPU_LAUNCH_IDLE);
    atomic_store(&budget->failed, 0);
}

typedef struct shared_memory {
    pthread_mutex_t vnpu_schedule_mutex[VNPU_SCHED_MAX_VNPU];
    atomic_uint_fast64_t last_alive_time_ns[VNPU_SCHED_MAX_VNPU];
    atomic_uint_fast64_t last_kernel_time_ns[VNPU_SCHED_MAX_VNPU];
    atomic_uint_fast8_t vnpu_schedule_turn[VNPU_SCHED_MAX_VNPU];
    atomic_uint_fast8_t vnpu_core_limit_quota[VNPU_SCHED_MAX_VNPU];
    atomic_uint_fast8_t vnpu_priority[VNPU_SCHED_MAX_VNPU];
    atomic_int owner;
    atomic_uint_fast32_t magic_number;
    atomic_int slide_window_len;
    atomic_uint_fast64_t last_slide_window_time_ns;
    pthread_mutex_t npu_utilization_monitor_mutex;
    atomic_uint_fast64_t vnpu_quota_timeslice[VNPU_SCHED_MAX_VNPU];
    atomic_uint_fast64_t vnpu_cur_timeslice[VNPU_SCHED_MAX_VNPU];
    atomic_uint_fast8_t vnpu_quota_injected_for_turn[VNPU_SCHED_MAX_VNPU];
    atomic_int vnpu_sync_expected[VNPU_SCHED_MAX_VNPU];
    atomic_int vnpu_sync_completed[VNPU_SCHED_MAX_VNPU];
    atomic_uint_fast64_t vnpu_turn_max_sync[VNPU_SCHED_MAX_VNPU];
    atomic_uint_fast64_t vnpu_timeslice_for_turn[VNPU_SCHED_MAX_VNPU];
    vnpu_launch_budget_t launch_budget[VNPU_SCHED_MAX_VNPU];
} vnpu_time_slice_sched_t;
#endif
