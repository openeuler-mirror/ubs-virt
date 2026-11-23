/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of the License at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include "launch_budget.h"

#include <errno.h>
#include <stdatomic.h>
#include <time.h>

#include "npu_manager.h"
#include "vnpu_stats.h"

static vnpu_launch_budget_t g_local_state;
static vnpu_launch_budget_t *g_state = &g_local_state;
static vnpu_time_slice_sched_t *g_sched;
static uint8_t g_vnpu;
static atomic_uint_fast64_t g_in_flight;
static atomic_uint_fast64_t g_attached_epoch;
static atomic_uint_fast64_t g_reported_epoch;
static atomic_bool g_borrowing;

static uint64_t budget_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void launch_budget_bind(vnpu_time_slice_sched_t *sched, uint8_t vnpu_id)
{
    g_sched = sched;
    g_vnpu = vnpu_id;
    g_state = sched != NULL ? &sched->launch_budget[vnpu_id] : &g_local_state;
    launch_budget_reset();
}

int launch_budget_lock(void)
{
    if (g_sched == NULL) {
        return 0;
    }
    int rc = pthread_mutex_lock(&g_sched->vnpu_schedule_mutex[g_vnpu]);
    if (rc == EOWNERDEAD) {
        /* A crashed holder makes this turn unsuitable for refund or feedback. */
        atomic_store(&g_state->failed, 1);
        if (atomic_load(&g_state->phase) == VNPU_LAUNCH_OPEN) {
            atomic_store(&g_state->phase, VNPU_LAUNCH_DRAIN);
        }
        rc = pthread_mutex_consistent(&g_sched->vnpu_schedule_mutex[g_vnpu]);
    }
    return rc;
}

void launch_budget_unlock(void)
{
    if (g_sched != NULL) {
        pthread_mutex_unlock(&g_sched->vnpu_schedule_mutex[g_vnpu]);
    }
}

static void launch_budget_attach_turn(void)
{
    atomic_store(&g_in_flight, 0);
    atomic_store(&g_attached_epoch, atomic_load(&g_state->epoch));
}

bool launch_budget_join_turn(uint64_t now)
{
    uint64_t epoch = atomic_load(&g_state->epoch);
    if (g_sched == NULL || atomic_load(&g_sched->owner) != g_vnpu ||
        atomic_load(&g_sched->vnpu_schedule_turn[g_vnpu]) != (uint8_t)atomic_load(&g_state->turn_id) ||
        atomic_load(&g_state->phase) != VNPU_LAUNCH_OPEN || atomic_load(&g_attached_epoch) == epoch ||
        (atomic_load(&g_state->deadline_ns) != atomic_load(&g_state->start_ns) &&
         now >= atomic_load(&g_state->deadline_ns))) {
        return false;
    }
    atomic_fetch_add(&g_sched->vnpu_sync_expected[g_vnpu], 1);
    launch_budget_attach_turn();
    return true;
}

bool launch_budget_report_drain(uint64_t epoch, bool complete)
{
    if (g_sched == NULL || atomic_load(&g_state->epoch) != epoch ||
        atomic_load(&g_sched->vnpu_schedule_turn[g_vnpu]) != (uint8_t)atomic_load(&g_state->turn_id) ||
        atomic_load(&g_state->phase) != VNPU_LAUNCH_DRAIN || atomic_load(&g_attached_epoch) != epoch ||
        atomic_load(&g_reported_epoch) == epoch) {
        return false;
    }
    if (!complete || atomic_load(&g_in_flight) != 0) {
        atomic_store(&g_state->failed, 1);
    }
    atomic_fetch_add(&g_sched->vnpu_sync_completed[g_vnpu], 1);
    atomic_store(&g_reported_epoch, epoch);
    return true;
}

void launch_budget_set_borrowing(bool borrowing)
{
    atomic_store(&g_borrowing, borrowing);
}

static bool borrowing_window(void)
{
    return g_sched != NULL && atomic_load(&g_borrowing) && atomic_load(&g_sched->owner) != g_vnpu;
}

static bool gate_open_locked(void)
{
    if (g_sched == NULL || borrowing_window()) {
        return true;
    }
    int phase = atomic_load(&g_state->phase);
    return atomic_load(&g_sched->owner) == g_vnpu &&
           atomic_load(&g_sched->vnpu_schedule_turn[g_vnpu]) == (uint8_t)atomic_load(&g_state->turn_id) &&
           atomic_load(&g_attached_epoch) == atomic_load(&g_state->epoch) &&
           (phase == VNPU_LAUNCH_DRAIN ||
            (phase == VNPU_LAUNCH_OPEN && budget_now() < atomic_load(&g_state->deadline_ns)));
}

bool launch_budget_gate_open(void)
{
    if (launch_budget_lock() != 0) {
        return false;
    }
    bool open = gate_open_locked();
    launch_budget_unlock();
    return open;
}

uint64_t launch_budget_calculate(uint64_t timeslice_ns, int64_t avg_duration_ns, bool *duration_fallback)
{
    if (duration_fallback != NULL) {
        *duration_fallback = false;
    }
    if (timeslice_ns == 0) {
        return 0;
    }

    uint64_t duration_ns = 0;
    if (avg_duration_ns > 0) {
        duration_ns = (uint64_t)avg_duration_ns;
    } else {
        duration_ns = DEFAULT_KERNEL_AVE_EXE_TIME;
        if (duration_fallback != NULL) {
            *duration_fallback = true;
        }
    }

    uint64_t budget = timeslice_ns / duration_ns;
    if (budget < LAUNCH_BUDGET_MIN) {
        return LAUNCH_BUDGET_MIN;
    }
    if (budget > LAUNCH_BUDGET_MAX) {
        return LAUNCH_BUDGET_MAX;
    }
    return budget;
}

void launch_budget_prepare_turn(uint8_t turn_id, uint64_t timeslice_ns, int32_t sched_policy, int64_t avg_duration_ns)
{
    /* Publish enabled last so a future hot-path reader cannot observe a partially reset turn. */
    atomic_store(&g_state->enabled, false);
    atomic_fetch_add(&g_state->epoch, 1);
    atomic_store(&g_state->launch_count, 0);
    atomic_store(&g_state->yield_requested, false);
    atomic_store(&g_state->model_fallbacks, 0);
    atomic_store(&g_state->turn_id, turn_id);
    atomic_store(&g_state->drain_launches, 0);
    atomic_store(&g_state->failed, 0);
    uint64_t now = budget_now();
    atomic_store(&g_state->start_ns, now);
    atomic_store(&g_state->deadline_ns, now + timeslice_ns);
    atomic_store(&g_state->stop_ns, now);
    atomic_store(&g_state->phase, VNPU_LAUNCH_OPEN);
    if (g_sched == NULL) {
        launch_budget_attach_turn();
    }

    if ((sched_policy != SCHED_POLICY_FIXED_SHARE && sched_policy != SCHED_POLICY_ELASTIC) || timeslice_ns == 0) {
        atomic_store(&g_state->launch_budget, 0);
        atomic_store(&g_state->duration_ns, 0);
        atomic_store(&g_state->duration_fallback, false);
        return;
    }

    bool duration_fallback = false;
    uint64_t budget = launch_budget_calculate(timeslice_ns, avg_duration_ns, &duration_fallback);
    uint64_t alpha_per_mille = atomic_load(&g_state->alpha_per_mille);
    if (alpha_per_mille < LAUNCH_BUDGET_ALPHA_MIN) {
        /* A fresh (zero) state starts neutral; stale sub-minimum values are clamped. */
        alpha_per_mille = alpha_per_mille == 0 ? LAUNCH_BUDGET_ALPHA_SCALE : LAUNCH_BUDGET_ALPHA_MIN;
        atomic_store(&g_state->alpha_per_mille, alpha_per_mille);
    }
    budget = budget * alpha_per_mille / LAUNCH_BUDGET_ALPHA_SCALE;
    if (budget < LAUNCH_BUDGET_MIN) {
        budget = LAUNCH_BUDGET_MIN;
    }
    if (budget > LAUNCH_BUDGET_MAX) {
        budget = LAUNCH_BUDGET_MAX;
    }
    atomic_store(&g_state->launch_budget, budget);
    atomic_store(&g_state->duration_ns, avg_duration_ns > 0 ? (uint64_t)avg_duration_ns : DEFAULT_KERNEL_AVE_EXE_TIME);
    atomic_store(&g_state->duration_fallback, duration_fallback);
    atomic_store(&g_state->enabled, true);
}

static bool try_admit_locked(uint64_t weight, launch_budget_ticket_t *ticket)
{
    /* The caller pre-zeroes *ticket, so every early return leaves it unreserved. */
    if (!gate_open_locked()) {
        return false;
    }
    if (borrowing_window()) {
        return true;
    }
    /* A sibling may close the shared turn before this process closes its local gate. */
    if (g_sched != NULL && atomic_load(&g_state->phase) == VNPU_LAUNCH_DRAIN) {
        return false;
    }
    if (!atomic_load(&g_state->enabled)) {
        return true;
    }
    if (ticket == NULL) {
        return false;
    }

    uint64_t budget = atomic_load(&g_state->launch_budget);
    uint64_t count = atomic_load(&g_state->launch_count);
    while (count < budget) {
        /* An oversized model must run alone; otherwise it could never be admitted. */
        uint64_t next = weight > budget - count ? (count == 0 ? weight : count) : count + weight;
        if (next == count) {
            break;
        }
        if (atomic_compare_exchange_weak(&g_state->launch_count, &count, next)) {
            atomic_fetch_add(&g_in_flight, 1);
            ticket->epoch = atomic_load(&g_attached_epoch);
            ticket->reserved = true;
            if (next >= budget) {
                atomic_store(&g_state->yield_requested, true);
            }
            return true;
        }
    }

    atomic_store(&g_state->yield_requested, true);
    return false;
}

bool launch_budget_try_admit_weighted(uint64_t weight, launch_budget_ticket_t *ticket)
{
    if (ticket != NULL) {
        *ticket = (launch_budget_ticket_t){0};
    }
    if (launch_budget_lock() != 0) {
        return false;
    }
    bool admitted = try_admit_locked(weight, ticket);
    launch_budget_unlock();
    return admitted;
}

void launch_budget_record_model_fallback(void)
{
    if (launch_budget_lock() == 0) {
        if (!borrowing_window() &&
            (g_sched == NULL || atomic_load(&g_attached_epoch) == atomic_load(&g_state->epoch))) {
            atomic_fetch_add(&g_state->model_fallbacks, 1);
        }
        launch_budget_unlock();
    }
}

void launch_budget_complete(launch_budget_ticket_t *ticket)
{
    if (ticket == NULL || !ticket->reserved) {
        return;
    }
    uint64_t in_flight = atomic_load(&g_in_flight);
    while (in_flight > 0 && ticket->epoch == atomic_load(&g_attached_epoch)) {
        if (atomic_compare_exchange_weak(&g_in_flight, &in_flight, in_flight - 1)) {
            break;
        }
    }
    ticket->reserved = false;
}

bool launch_budget_should_yield(uint8_t turn_id)
{
    bool matching = (uint8_t)atomic_load(&g_state->turn_id) == turn_id;
    if (g_sched != NULL) {
        /* Budget-triggered yield waits for local Hook cleanup; the scheduler deadline still bounds the turn. */
        return !matching || atomic_load(&g_attached_epoch) != atomic_load(&g_state->epoch) ||
               atomic_load(&g_state->phase) != VNPU_LAUNCH_OPEN ||
               (atomic_load(&g_state->yield_requested) && atomic_load(&g_in_flight) == 0);
    }
    return matching && atomic_load(&g_state->enabled) && atomic_load(&g_state->yield_requested) &&
           atomic_load(&g_in_flight) == 0;
}

void launch_budget_apply_feedback(uint8_t turn_id, uint64_t elapsed_ns, bool drain_complete)
{
    if ((uint8_t)atomic_load(&g_state->turn_id) != turn_id || elapsed_ns == 0 || !drain_complete ||
        atomic_load(&g_state->duration_fallback) || atomic_load(&g_state->model_fallbacks) != 0 ||
        atomic_load(&g_state->failed) || atomic_load(&g_state->drain_launches) != 0) {
        return;
    }

    uint64_t count = atomic_load(&g_state->launch_count);
    uint64_t duration_ns = atomic_load(&g_state->duration_ns);
    if (count == 0 || duration_ns == 0) {
        return;
    }

    __uint128_t sample_value = (__uint128_t)count * duration_ns * LAUNCH_BUDGET_ALPHA_SCALE / elapsed_ns;
    uint64_t sample = sample_value < LAUNCH_BUDGET_ALPHA_MIN ? LAUNCH_BUDGET_ALPHA_MIN :
                      sample_value > LAUNCH_BUDGET_ALPHA_MAX ? LAUNCH_BUDGET_ALPHA_MAX :
                                                               (uint64_t)sample_value;
    uint64_t current = atomic_load(&g_state->alpha_per_mille);
    current = current < LAUNCH_BUDGET_ALPHA_MIN ? LAUNCH_BUDGET_ALPHA_MIN : current;
    uint64_t next =
        (current * (LAUNCH_BUDGET_ALPHA_SMOOTHING_DIVISOR - 1) + sample) / LAUNCH_BUDGET_ALPHA_SMOOTHING_DIVISOR;
    atomic_store(&g_state->alpha_per_mille, next);
}

void launch_budget_close_turn(uint64_t epoch, uint64_t stopped_ns)
{
    int phase = atomic_load(&g_state->phase);
    if (atomic_load(&g_state->epoch) != epoch || (phase != VNPU_LAUNCH_OPEN && phase != VNPU_LAUNCH_DRAIN)) {
        return;
    }
    atomic_store(&g_state->phase, VNPU_LAUNCH_DRAIN);
    if (stopped_ns > atomic_load(&g_state->stop_ns)) {
        atomic_store(&g_state->stop_ns, stopped_ns);
    }
}

uint64_t launch_budget_refund_turn(uint64_t epoch, bool complete)
{
    if (atomic_load(&g_state->epoch) != epoch || atomic_load(&g_state->phase) != VNPU_LAUNCH_DRAIN) {
        return 0;
    }
    uint64_t start = atomic_load(&g_state->start_ns);
    uint64_t deadline = atomic_load(&g_state->deadline_ns);
    bool siblings_complete = g_sched == NULL || atomic_load(&g_sched->vnpu_sync_completed[g_vnpu]) >=
                                                    atomic_load(&g_sched->vnpu_sync_expected[g_vnpu]);
    bool reliable = complete && siblings_complete && !atomic_load(&g_state->failed) &&
                    atomic_load(&g_state->drain_launches) == 0;
    uint64_t stop = atomic_load(&g_state->stop_ns);
    /* Refund only the common window left after the last participant stopped.
     * Drain debt is still charged by the scheduler's original max-sync logic. */
    uint64_t refund = reliable && atomic_load(&g_state->yield_requested) && stop < deadline ? deadline - stop : 0;
    launch_budget_apply_feedback((uint8_t)atomic_load(&g_state->turn_id), stop - start, reliable);
    atomic_store(&g_state->enabled, false);
    atomic_store(&g_state->phase, VNPU_LAUNCH_IDLE);
    return refund;
}

void launch_budget_reset(void)
{
    atomic_store(&g_in_flight, 0);
    atomic_store(&g_attached_epoch, 0);
    atomic_store(&g_reported_epoch, 0);
    atomic_store(&g_borrowing, false);
    if (g_sched == NULL) {
        uint64_t epoch = atomic_load(&g_state->epoch) + 1;
        vnpu_launch_budget_init(g_state);
        atomic_store(&g_state->epoch, epoch);
    }
}

void launch_budget_get_snapshot(launch_budget_snapshot_t *snapshot)
{
    if (snapshot == NULL) {
        return;
    }
    snapshot->launch_count = atomic_load(&g_state->launch_count);
    snapshot->launch_budget = atomic_load(&g_state->launch_budget);
    snapshot->in_flight_launches = atomic_load(&g_in_flight);
    snapshot->duration_ns = atomic_load(&g_state->duration_ns);
    snapshot->alpha_per_mille = atomic_load(&g_state->alpha_per_mille);
    snapshot->model_fallbacks = atomic_load(&g_state->model_fallbacks);
    snapshot->epoch = atomic_load(&g_state->epoch);
    snapshot->drain_launches = atomic_load(&g_state->drain_launches);
    snapshot->turn_id = (uint8_t)atomic_load(&g_state->turn_id);
    snapshot->enabled = atomic_load(&g_state->enabled);
    snapshot->yield_requested = atomic_load(&g_state->yield_requested);
    snapshot->duration_fallback = atomic_load(&g_state->duration_fallback);
}
