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
#ifndef ENPU_LAUNCH_BUDGET_H
#define ENPU_LAUNCH_BUDGET_H

#include <stdbool.h>
#include <stdint.h>
#include "vnpu_sched_shm_abi.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* Internal safety bounds. They are not user-facing configuration. */
#define LAUNCH_BUDGET_MIN 1ULL
#define LAUNCH_BUDGET_MAX 65536ULL
#define LAUNCH_BUDGET_ALPHA_SCALE 1000ULL
#define LAUNCH_BUDGET_ALPHA_MIN (LAUNCH_BUDGET_ALPHA_SCALE / 4ULL)
#define LAUNCH_BUDGET_ALPHA_MAX (8ULL * LAUNCH_BUDGET_ALPHA_SCALE)
#define LAUNCH_BUDGET_ALPHA_SMOOTHING_DIVISOR 4ULL

typedef struct {
    uint64_t launch_count;
    uint64_t launch_budget;
    uint64_t in_flight_launches;
    uint64_t duration_ns;
    uint64_t alpha_per_mille;
    uint64_t model_fallbacks;
    uint64_t epoch;
    uint64_t drain_launches;
    uint8_t turn_id;
    bool enabled;
    bool yield_requested;
    bool duration_fallback;
} launch_budget_snapshot_t;

typedef struct {
    uint64_t epoch;
    bool reserved;
} launch_budget_ticket_t;

uint64_t launch_budget_calculate(uint64_t timeslice_ns, int64_t avg_duration_ns, bool *duration_fallback);
void launch_budget_prepare_turn(uint8_t turn_id, uint64_t timeslice_ns, int32_t sched_policy, int64_t avg_duration_ns);
bool launch_budget_try_admit_weighted(uint64_t weight, launch_budget_ticket_t *ticket);
void launch_budget_record_model_fallback(void);
void launch_budget_complete(launch_budget_ticket_t *ticket);
bool launch_budget_should_yield(uint8_t turn_id);
void launch_budget_apply_feedback(uint8_t turn_id, uint64_t elapsed_ns, bool drain_complete);
void launch_budget_reset(void);
void launch_budget_get_snapshot(launch_budget_snapshot_t *snapshot);

/* Bind before starting scheduler/launch threads. Reset/exit only clears local state. */
void launch_budget_bind(vnpu_time_slice_sched_t *sched, uint8_t vnpu_id);
int launch_budget_lock(void);
void launch_budget_unlock(void);
bool launch_budget_join_turn(uint64_t now);
bool launch_budget_report_drain(uint64_t epoch, bool complete);
void launch_budget_set_borrowing(bool borrowing);
bool launch_budget_gate_open(void);
/* Caller holds vnpu_schedule_mutex for prepare, close, and refund. */
void launch_budget_close_turn(uint64_t epoch, uint64_t stopped_ns);
uint64_t launch_budget_refund_turn(uint64_t epoch, bool complete);

#if defined(__cplusplus)
}
#endif

#endif /* ENPU_LAUNCH_BUDGET_H */
