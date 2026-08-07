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
#include "core_limiter.h"
#include "common.h"
#include "dcmi_wrapper.h"
#include "hash_map.h"
#include "npu_manager.h"
#include "runtime_hook.h"
#include "utils.h"

vnpu_time_slice_sched_t *g_vnpu_sched_context = NULL;
uint8_t g_vnpu_id = 0;
volatile int g_terminate = 0;
atomic_bool g_sched_locking = false;
atomic_int hasModelExecuteSync = 0;
pthread_mutex_t g_sched_mutex = PTHREAD_MUTEX_INITIALIZER;
atomic_bool g_monitor_init = false;

cache_streams_t g_cache_streams = {.num_streams = 0, .streams = {NULL}};

HashMap *stream_map = NULL;
HashMap *event_map = NULL;

uint64_t ns_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * NS_PER_S + (uint64_t)ts.tv_nsec;
}

/// The input must be less than 1000000000.
void ns_sleep(uint64_t ns)
{
    struct timespec req;
    struct timespec rem;
    req.tv_sec = 0;
    req.tv_nsec = ns;
    while (nanosleep(&req, &rem) == -1) {
        if (errno == EINTR) {
            req = rem;
        } else {
            break;
        }
    }
}

void restore_streams(rtStream_t stream)
{
    if (stream == NULL) {
        return;
    }

    if (hashmap_contains(stream_map, (void *)stream)) {
        return;
    }

    if (g_cache_streams.num_streams >= MAX_STREAMS_PER_PROCESS) {
        LOG_ERROR("Failed to add stream %p to the cache. Maximum capacity (%d) reached.", (void *)stream,
                  MAX_STREAMS_PER_PROCESS);
        return;
    }

    g_cache_streams.streams[g_cache_streams.num_streams++] = stream;
    int ret = hashmap_put(stream_map, (void *)stream, NULL, false);
    CHECK_COND_RETURN(ret == -1, "Failed to put stream %p to the hash map.", (void *)stream);
    LOG_DEBUG("Stream %p is added in stream hash map.", (void *)stream);
    return;
}

void add_stream(rtStream_t stream)
{
    if (hashmap_contains(stream_map, (void *)stream)) {
        return;
    }
    int ret = hashmap_put(stream_map, (void *)stream, NULL, false);
    CHECK_COND_RETURN(ret == -1, "Failed to put stream %p to the hash map.", (void *)stream);
    LOG_DEBUG("Stream %p is added in stream hash map.", (void *)stream);
    return;
}

void core_limiter(rtStream_t stream, core_function func, void *param)
{
    // when schedule policy is 3
    if (!is_core_limit()) {
        return;
    }
    while (!g_terminate) {
        // g_sched_locking is a atomic_int for scheduler thread obtain lock with high priority
        if (atomic_load(&g_sched_locking)) {
            ns_sleep(WAITING_SLEEP_PERIOD);
            continue;
        }
        // waiting for mutex == waiting for launch task
        int rc = pthread_mutex_lock(&g_sched_mutex);
        CHECK_COND_RETURN(rc != 0, "Failed to lock mutex, error code=%d.", rc);
        // double-check g_sched_locking
        if (atomic_load(&g_sched_locking)) {
            pthread_mutex_unlock(&g_sched_mutex);
            ns_sleep(WAITING_SLEEP_PERIOD);
            continue;
        }
        // The delivered stream needs to be recorded because the execution time needs to be collected later.
        restore_streams(stream);
        if (func != NULL) {
            func(param, stream);
        }
        pthread_mutex_unlock(&g_sched_mutex);
        // Recording time when the last task was delivered, which is used for schedule policy 2.
        atomic_store(&g_vnpu_sched_context->last_kernel_time_ns[g_vnpu_id], ns_now());
        return;
    }

    return;
}

bool check_timeout(atomic_uint_fast64_t *timestamp, uint64_t timeout_period)
{
    uint64_t last = atomic_load(timestamp);
    uint64_t now = ns_now();
    // Reboot will recount ns_now() from 0 but not reset timestamp which stored in shared memory.
    // This check is necessary for the problem described above.
    if (last < now) {
        return (now - last <= timeout_period);
    } else {
        return (last - now <= timeout_period);
    }
}

bool is_vnpu_alive(int vnpu_id)
{
    if (vnpu_id < 0 || vnpu_id >= MAX_VNPU) {
        return false;
    }
    return check_timeout(&g_vnpu_sched_context->last_alive_time_ns[vnpu_id], VNPU_TIMEOUT_PERIOD);
}

inline bool vnpu_has_work(int vnpu_id)
{
    return check_timeout(&g_vnpu_sched_context->last_kernel_time_ns[vnpu_id], VNPU_NO_TASK_TIMEOUT_PERIOD);
}

void vnpu_idling(void)
{
    int npu_core_limit_quota = 0;
    for (int i = 0; i < MAX_VNPU; ++i) {
        if (is_vnpu_alive(i)) {
            npu_core_limit_quota += atomic_load(&g_vnpu_sched_context->vnpu_core_limit_quota[i]);
        }
        if (npu_core_limit_quota > HUNDRED_PERCENT) {
            return;
        }
    }
    ns_sleep((HUNDRED_PERCENT - npu_core_limit_quota) * VNPU_SCHEULE_PERIOD / HUNDRED_PERCENT);
}

int select_next_owner(int vnpu_id)
{
    int next_vnpu_id = -1;

    for (int i = 1; i <= MAX_VNPU; ++i) {
        if (is_vnpu_alive((vnpu_id + i) % MAX_VNPU)) {
            next_vnpu_id = (vnpu_id + i) % MAX_VNPU;
            break;
        }
    }

    return next_vnpu_id;
}

void set_vnpu_and_idle(int vnpu_id, int next_vnpu_id)
{
    if (next_vnpu_id == -1) {
        return;
    }
    if (get_sched_policy() == SCHED_POLICY_FIXED_SHARE && next_vnpu_id <= vnpu_id) {
        vnpu_idling();
    }
    atomic_store(&g_vnpu_sched_context->owner, next_vnpu_id);
}

void synchronize_and_clear_streams(void)
{
    int remaining_count = 0;
    for (int i = 0; i < g_cache_streams.num_streams; ++i) {
        rtStream_t stm = g_cache_streams.streams[i];
        bool capture = 0;
        int rc = hashmap_get_capture_status(stream_map, (void *)stm, &capture);
        CHECK_COND_RETURN(rc == -1, "Failed to get stream %p capture_status from the hash map.", (void *)stm);
        if (capture) {
            LOG_DEBUG("Stream %p is in capture, skip synchronization and clear.", (void *)stm);
            g_cache_streams.streams[remaining_count++] = stm;
            continue;
        }
        LOG_DEBUG("Stream %p is being synchronized.", (void *)stm);
        RUNTIME_HOOK_CALL(rt_library_entry, rtStreamSynchronize, stm);
        rc = hashmap_remove(stream_map, (void *)stm);
        CHECK_COND_RETURN(rc == -1, "Failed to remove stream %p from the hash map.", (void *)stm);
    }
    g_cache_streams.num_streams = remaining_count;
}

// without locks, multiple processes are executed in parallel.
static uint64_t sync_own_streams_and_measure_ns(void)
{
    uint64_t begin = ns_now();
    while (atomic_load(&hasModelExecuteSync) > 0) {
        ns_sleep(WAITING_SLEEP_PERIOD);
    }
    synchronize_and_clear_streams();
    return ns_now() - begin;
}

// wait to finish parallel stream synchronization with all sibling processes of vNPU.
static void wait_for_sibling_sync(uint8_t vnpu_id, uint8_t turn_id)
{
    uint64_t wait_deadline = ns_now() + SYNC_WAIT_TIMEOUT_NS;
    while (ns_now() < wait_deadline) {
        // re-check turn
        if (atomic_load(&g_vnpu_sched_context->vnpu_schedule_turn[vnpu_id]) != turn_id) {
            LOG_DEBUG("vNPU %d pid %d: wait detected turn advanced (turn_id=%u), exit early.", vnpu_id, (int)getpid(),
                      turn_id);
            return;
        }
        int expected = atomic_load(&g_vnpu_sched_context->vnpu_sync_expected[vnpu_id]);
        int completed = atomic_load(&g_vnpu_sched_context->vnpu_sync_completed[vnpu_id]);
        if (completed >= expected) {
            return;
        }
        ns_sleep(WAITING_SLEEP_PERIOD);
    }
    int expected = atomic_load(&g_vnpu_sched_context->vnpu_sync_expected[vnpu_id]);
    int completed = atomic_load(&g_vnpu_sched_context->vnpu_sync_completed[vnpu_id]);
    LOG_WARN("vNPU %d pid %d: sync wait timeout, completed=%d expected=%d. "
             "Slow sibling's sync not counted in debt this turn.",
             vnpu_id, (int)getpid(), completed, expected);
}

// multiple processes fetch and consume time slices from shared memory
uint64_t add_and_consume_time_slice(uint8_t *turn_id, int *next_vnpu_id)
{
    // processes of the same vNPU cannot inject quota/deport timeslice concurrently
    int rc = pthread_mutex_lock(&g_vnpu_sched_context->vnpu_schedule_mutex[g_vnpu_id]);
    if (rc == EOWNERDEAD) {
        LOG_INFO("Consumer mutex owner died; taking over for vNPU %d.", g_vnpu_id);
        pthread_mutex_consistent(&g_vnpu_sched_context->vnpu_schedule_mutex[g_vnpu_id]);
    } else if (rc != 0) {
        LOG_WARN("Failed to lock vnpu_schedule_mutex, rc=%d.", rc);
        *turn_id = atomic_load(&g_vnpu_sched_context->vnpu_schedule_turn[g_vnpu_id]);
        *next_vnpu_id = -1;
        return 0;
    }

    uint8_t cur_turn = atomic_load(&g_vnpu_sched_context->vnpu_schedule_turn[g_vnpu_id]);

    // only one process can operate this round of quota
    if (cur_turn != atomic_load(&g_vnpu_sched_context->vnpu_quota_injected_for_turn[g_vnpu_id])) {
        atomic_store(&g_vnpu_sched_context->vnpu_sync_expected[g_vnpu_id], 0);
        atomic_store(&g_vnpu_sched_context->vnpu_sync_completed[g_vnpu_id], 0);
        atomic_store(&g_vnpu_sched_context->vnpu_turn_max_sync[g_vnpu_id], 0ULL);

        uint64_t quota = atomic_load(&g_vnpu_sched_context->vnpu_quota_timeslice[g_vnpu_id]);
        atomic_fetch_add(&g_vnpu_sched_context->vnpu_cur_timeslice[g_vnpu_id], quota);

        int64_t cur_after_inject = (int64_t)atomic_load(&g_vnpu_sched_context->vnpu_cur_timeslice[g_vnpu_id]);
        uint64_t ts = cur_after_inject > 0 ? (uint64_t)cur_after_inject : 0ULL;
        if (ts > 0) {
            atomic_fetch_sub(&g_vnpu_sched_context->vnpu_cur_timeslice[g_vnpu_id], ts);
        }
        // for multi processes: record the current round's time slice quota into the shared memory
        atomic_store(&g_vnpu_sched_context->vnpu_timeslice_for_turn[g_vnpu_id], ts);
        atomic_store(&g_vnpu_sched_context->vnpu_quota_injected_for_turn[g_vnpu_id], cur_turn);

        LOG_DEBUG("Quota %llu ns injected for vNPU %d turn %u; timeslice=%llu ns.", (unsigned long long)quota,
                  g_vnpu_id, cur_turn, (unsigned long long)ts);
    }

    atomic_fetch_add(&g_vnpu_sched_context->vnpu_sync_expected[g_vnpu_id], 1);

    *turn_id = cur_turn;
    int owner = atomic_load(&g_vnpu_sched_context->owner);
    *next_vnpu_id = select_next_owner(owner);

    // multiple processes consume time slices in parallel
    pthread_mutex_unlock(&g_vnpu_sched_context->vnpu_schedule_mutex[g_vnpu_id]);
    uint64_t ts = atomic_load(&g_vnpu_sched_context->vnpu_timeslice_for_turn[g_vnpu_id]);
    if (ts > 0) {
        atomic_store(&g_sched_locking, false);
        uint64_t end = ns_now() + ts;
        while (ns_now() < end && !g_terminate) {
            if (atomic_load(&g_vnpu_sched_context->owner) != g_vnpu_id) {
                break;
            }
            ns_sleep(WAITING_SLEEP_PERIOD);
        }
        atomic_store(&g_sched_locking, true);
    }

    return ts;
}

void *vnpu_scheduler_flush_thread(void *arg)
{
    (void)arg;
    while (!g_terminate) {
        uint64_t now = ns_now();
        atomic_store(&g_vnpu_sched_context->last_alive_time_ns[g_vnpu_id], now);
        ns_sleep(VNPU_FLUSH_PERIOD);
    }
    return NULL;
}

int calculate_alive_vnpu_num(void)
{
    int count = 0;
    for (size_t i = 0; i < MAX_VNPU; i++) {
        if (is_vnpu_alive(i)) {
            count++;
        }
    }
    return count;
}

void *npu_utilization_monitor_thread(void *arg)
{
    (void)arg;
    int rc = pthread_mutex_lock(&g_vnpu_sched_context->npu_utilization_monitor_mutex);
    if (rc == EOWNERDEAD) {
        pthread_mutex_consistent(&g_vnpu_sched_context->npu_utilization_monitor_mutex);
    } else if (rc != 0) {
        LOG_WARN("Failed to obtain mutex lock, error code=%d.", rc);
        return NULL;
    }
    int owner = atomic_load(&g_vnpu_sched_context->owner);
    while (owner == g_vnpu_id) {
        if (check_timeout(&g_vnpu_sched_context->last_slide_window_time_ns, WATTING_SLIDE_WINDOW_TIMEOUT_PERIOD)) {
            ns_sleep(VNPU_FLUSH_PERIOD);
            owner = atomic_load(&g_vnpu_sched_context->owner);
            continue;
        }
        unsigned int utilization_rate = 0;
        uint64_t begin = ns_now();
        atomic_store(&g_vnpu_sched_context->last_slide_window_time_ns, begin);
        int ret =
            enpu_dcmi_get_device_utilization_rate(get_logic_id(), get_card_id(), get_device_id(), &utilization_rate);
        if (ret != ENPU_SUCCESS) {
            LOG_ERROR("DCMI call failed with ret: %d.", ret);
            owner = atomic_load(&g_vnpu_sched_context->owner);
            continue;
        }

        uint64_t now = ns_now();
        uint64_t diff_ns = now - begin;
        if (diff_ns > DCMI_TIMEOUT_THRESHOLD) {
            LOG_DEBUG("The DCMI interface is overloaded, reuse the NPU utilization status from the last time.");
            owner = atomic_load(&g_vnpu_sched_context->owner);
            continue;
        }

        static int high_load_streak = 0;
        static int low_load_streak = 0;
        int current_window = atomic_load(&g_vnpu_sched_context->slide_window_len);
        int new_window = current_window;

        if (utilization_rate > UTILIZATION_RATE_MAX) {
            low_load_streak = 0;
            high_load_streak++;
            if (high_load_streak >= MAX_STREAK && current_window > 0) {
                new_window = current_window - 1;
                high_load_streak = 0;
                LOG_DEBUG("Utilization high (%u%%), decreasing window to %d.", utilization_rate, new_window);
            }
        } else if (utilization_rate < UTILIZATION_RATE_MIN) {
            high_load_streak = 0;
            low_load_streak++;
            if (low_load_streak >= MAX_STREAK) {
                int max_len = calculate_alive_vnpu_num() - 1;
                max_len = (max_len < 0) ? 0 : max_len;
                if (current_window < max_len) {
                    new_window = current_window + 1;
                    LOG_DEBUG("Utilization low (%u%%), increasing window to %d (max:%d).", utilization_rate, new_window,
                              max_len);
                }
                low_load_streak = 0;
            }
        } else {
            high_load_streak = 0;
            low_load_streak = 0;
        }

        if (new_window != current_window) {
            atomic_store(&g_vnpu_sched_context->slide_window_len, new_window);
        }
        ns_sleep(VNPU_FLUSH_PERIOD);
        owner = atomic_load(&g_vnpu_sched_context->owner);
    }
    atomic_store(&g_monitor_init, false);
    pthread_mutex_unlock(&g_vnpu_sched_context->npu_utilization_monitor_mutex);
    return NULL;
}

bool slide_window_check(int owner)
{
    int slide_windows_len = atomic_load(&g_vnpu_sched_context->slide_window_len);

    for (int i = 1; i <= MAX_VNPU && slide_windows_len > 0; ++i) {
        int next_vnpu = (owner + i) % MAX_VNPU;
        if (next_vnpu == g_vnpu_id) {
            return true;
        }
        // The slide window only contains alive vnpu.
        if (is_vnpu_alive(next_vnpu)) {
            slide_windows_len -= 1;
        }
    }
    return false;
}

void check_and_borrow_timeslice(int owner)
{
    if (owner == g_vnpu_id) {
        // Check and update slide_window_len, no borrow here
        if (!atomic_load(&g_monitor_init)) {
            atomic_store(&g_monitor_init, true);
            pthread_t thread;
            int rc = pthread_create(&thread, NULL, npu_utilization_monitor_thread, NULL);
            CHECK_ERROR_CODE(rc, "Failed to create npu_utilization_monitor_thread.");
            pthread_detach(thread);
        }
    } else if (slide_window_check(owner)) { // Check and borrow timeslice
        atomic_store(&g_sched_locking, false);
        ns_sleep(BORROW_TIMESLICE_LENGTH); // borrow BORROW_TIMESLICE_LENGTH ns every time
        atomic_store(&g_sched_locking, true);
    }
}

// Scheduling main thread
void *vnpu_scheduler_thread(void *arg)
{
    (void)arg;
    uint8_t turn_id = -1;
    int next_vnpu_id = -1;
    // g_sched_locking = true : user can not launch task by core_limiter
    atomic_store(&g_sched_locking, true);
    while (!g_terminate) {
        // Distributed thread scheduling.
        // Scheduling is performed only when the owner is the current vnpu or the owner is disabled.
        int owner = atomic_load(&g_vnpu_sched_context->owner);

        // ELASTIC will consider borrow timeslice
        if (get_sched_policy() == SCHED_POLICY_ELASTIC) {
            check_and_borrow_timeslice(owner);
        }

        if (owner != g_vnpu_id) {
            if (!is_vnpu_alive(owner)) {
                int vnpu_id = atomic_load(&g_vnpu_sched_context->owner);
                set_vnpu_and_idle(vnpu_id, select_next_owner(vnpu_id));
            }
            ns_sleep(WAITING_SLEEP_PERIOD);
            continue;
        }

        // Consumption time slice. The lock is released to the user process within the specified time.
        uint64_t timeslice = add_and_consume_time_slice(&turn_id, &next_vnpu_id);

        if (timeslice > 0) {
            // without locks, multiple processes are executed in parallel.
            uint64_t my_sync_ns = sync_own_streams_and_measure_ns();
            if (atomic_load(&g_vnpu_sched_context->vnpu_schedule_turn[g_vnpu_id]) == turn_id) {
                atomic_fetch_max_uint64(&g_vnpu_sched_context->vnpu_turn_max_sync[g_vnpu_id], my_sync_ns);
                atomic_fetch_add(&g_vnpu_sched_context->vnpu_sync_completed[g_vnpu_id], 1);
                LOG_DEBUG("vNPU %d pid %d: parallel sync done, my_elapsed=%llu ns.", g_vnpu_id, (int)getpid(),
                          (unsigned long long)my_sync_ns);
            } else {
                // this process flow synchronization timeout, data discarded
                LOG_WARN("vNPU %d pid %d: sync took %llu ns, turn changed during sync (stale, skip).", g_vnpu_id,
                         (int)getpid(), (unsigned long long)my_sync_ns);
                continue;
            }
        } else {
            atomic_fetch_add(&g_vnpu_sched_context->vnpu_sync_completed[g_vnpu_id], 1);
        }

        // wait to finish parallel stream synchronization with all sibling processes of vNPU.
        wait_for_sibling_sync(g_vnpu_id, turn_id);

        // Only one thread is accepted.
        int rc = pthread_mutex_lock(&g_vnpu_sched_context->vnpu_schedule_mutex[g_vnpu_id]);
        if (rc == EOWNERDEAD) {
            LOG_INFO("The scheduling process has been detected to exit, and the scheduling is being taken over.");
            pthread_mutex_consistent(&g_vnpu_sched_context->vnpu_schedule_mutex[g_vnpu_id]);
        } else if (rc != 0) {
            LOG_WARN("Failed to obtain mutex lock, error code=%d.", rc);
            continue;
        }

        if (atomic_load(&g_vnpu_sched_context->vnpu_schedule_turn[g_vnpu_id]) == turn_id) {
            // Only the slice of the main process is considered.
            if (timeslice > 0) {
                uint64_t debt = atomic_load(&g_vnpu_sched_context->vnpu_turn_max_sync[g_vnpu_id]);
                (void)atomic_fetch_sub(&g_vnpu_sched_context->vnpu_cur_timeslice[g_vnpu_id], debt);
                LOG_DEBUG("Leader vNPU %d turn %u: applied debt=%llu ns (max of sibling syncs).", g_vnpu_id, turn_id,
                          (unsigned long long)debt);
            }

            set_vnpu_and_idle(atomic_load(&g_vnpu_sched_context->owner), next_vnpu_id);
            atomic_store(&g_vnpu_sched_context->vnpu_schedule_turn[g_vnpu_id], turn_id + 1);
        }
        pthread_mutex_unlock(&g_vnpu_sched_context->vnpu_schedule_mutex[g_vnpu_id]);
    }
    hashmap_destroy(stream_map);
    hashmap_destroy(event_map);
    return NULL;
}

void share_mem_init(vnpu_time_slice_sched_t *vnpu_sched_shm)
{
    g_vnpu_sched_context = vnpu_sched_shm;
    uint64_t begin = ns_now();

    while (!g_terminate) {
        if (atomic_load(&g_vnpu_sched_context->magic_number) == MAGIC_INITIALIZED) {
            return;
        }

        if (atomic_load(&g_vnpu_sched_context->magic_number) == MAGIC_INITIALIZING) {
            uint64_t now = ns_now();
            if (now - begin > VNPU_TIMEOUT_PERIOD) {
                atomic_store(&g_vnpu_sched_context->magic_number, MAGIC_UNINITIALIZED);
                begin = now;
            }
            ns_sleep(WAITING_SLEEP_PERIOD);
            continue;
        }

        atomic_store(&g_vnpu_sched_context->magic_number, MAGIC_INITIALIZING);
        atomic_store(&g_vnpu_sched_context->owner, -1);

        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);

        for (int i = 0; i < MAX_VNPU; ++i) {
            atomic_store(&g_vnpu_sched_context->last_alive_time_ns[i], 0ULL);
            atomic_store(&g_vnpu_sched_context->last_kernel_time_ns[i], 0ULL);
            atomic_store(&g_vnpu_sched_context->vnpu_core_limit_quota[i], 0);
            atomic_store(&g_vnpu_sched_context->vnpu_schedule_turn[i], 0);
            atomic_store(&g_vnpu_sched_context->vnpu_quota_timeslice[i], 0ULL);
            atomic_store(&g_vnpu_sched_context->vnpu_cur_timeslice[i], 0ULL);
            atomic_store(&g_vnpu_sched_context->vnpu_quota_injected_for_turn[i], (uint_fast8_t)0xFF);
            atomic_store(&g_vnpu_sched_context->vnpu_sync_expected[i], 0);
            atomic_store(&g_vnpu_sched_context->vnpu_sync_completed[i], 0);
            atomic_store(&g_vnpu_sched_context->vnpu_turn_max_sync[i], 0ULL);
            atomic_store(&g_vnpu_sched_context->vnpu_timeslice_for_turn[i], 0ULL);
            pthread_mutex_init(&g_vnpu_sched_context->vnpu_schedule_mutex[i], &attr);
        }

        pthread_mutex_init(&g_vnpu_sched_context->npu_utilization_monitor_mutex, &attr);
        pthread_mutexattr_destroy(&attr);
        atomic_store(&g_vnpu_sched_context->magic_number, MAGIC_INITIALIZED);
        return;
    }
}

int vnpu_scheduler_init(vnpu_time_slice_sched_t *vnpu_sched_shm)
{
    g_vnpu_sched_context = vnpu_sched_shm;
    g_vnpu_id = get_vnpu_id();

    uint8_t aicore_limit_percent = get_core_limit_quota();
    atomic_store(&g_vnpu_sched_context->vnpu_core_limit_quota[g_vnpu_id], aicore_limit_percent);
    uint64_t aicore_cur_timesilice = aicore_limit_percent * VNPU_SCHEULE_PERIOD / HUNDRED_PERCENT;
    atomic_store(&g_vnpu_sched_context->vnpu_quota_timeslice[g_vnpu_id], aicore_cur_timesilice);

    pthread_t vnpu_scheduler_tid;
    int rc = pthread_create(&vnpu_scheduler_tid, NULL, vnpu_scheduler_thread, NULL);
    CHECK_COND_RETURN_ERROR_CODE(rc != 0, "Failed to create vnpu scheduler thread.");

    pthread_t vnpu_alive_tid;
    rc = pthread_create(&vnpu_alive_tid, NULL, vnpu_scheduler_flush_thread, NULL);
    CHECK_COND_RETURN_ERROR_CODE(rc != 0, "Failed to create vnpu alive thread.");

    pthread_detach(vnpu_scheduler_tid);
    pthread_detach(vnpu_alive_tid);
    return ENPU_SUCCESS;
}

int aicore_limiter_initialize(void)
{
    int rc = ENPU_FAIL;
    vnpu_time_slice_sched_t *vnpu_sched_shm = NULL;
    vnpu_sched_shm = map_share_mem(get_vnpu_shm_id(), sizeof(*g_vnpu_sched_context));
    if (vnpu_sched_shm == NULL) {
        LOG_ERROR("Failed to mmap share memory.");
        return ENPU_FAIL;
    }

    share_mem_init(vnpu_sched_shm);

    rc = vnpu_scheduler_init(vnpu_sched_shm);
    CHECK_RETURN_ERROR_CODE(rc, "Failed to initialize vnpu scheduler.");

    stream_map = hashmap_create(MAX_STREAMS_PER_PROCESS);
    if (!stream_map) {
        LOG_ERROR("Stream hash map init failed.");
        return ENPU_FAIL;
    }

    event_map = hashmap_create(MAX_EVENT_PER_PROCESS);
    if (!event_map) {
        LOG_ERROR("Event hash map init failed.");
        hashmap_destroy(stream_map);
        return ENPU_FAIL;
    }
    return rc;
}

void set_stream_capture(void *param, rtStream_t stream)
{
    bool capture = *(bool *)param;
    if (!capture) {
        for (int i = 0; i < g_cache_streams.num_streams; ++i) {
            rtStream_t stm = g_cache_streams.streams[i];
            void *head_stream = NULL;
            int rc = hashmap_get_ptr(stream_map, (void *)stm, &head_stream);
            CHECK_COND_RETURN(rc == -1, "Failed to get stream %p ptr from the hash map.", (void *)stm);
            if (head_stream == (void *)stream) {
                LOG_DEBUG("Stream %p capture state set to: 0.", (void *)stream);
                rc = hashmap_put(stream_map, (void *)stm, NULL, false);
                CHECK_COND_RETURN(rc == -1, "Failed to put stream %p to the hash map.", (void *)stm);
            }
        }
    } else {
        int rc = hashmap_put(stream_map, (void *)stream, (void *)stream, capture);
        CHECK_COND_RETURN(rc == -1, "Failed to put stream %p to the hash map.", (void *)stream);
    }
    LOG_DEBUG("Stream %p capture state set to: %d.", (void *)stream, capture ? 1 : 0);
}

void set_event_wait_status(void *evt, rtStream_t stm)
{
    MapValue event_status;
    int rc = hashmap_get(event_map, evt, &event_status);
    CHECK_COND_RETURN(rc == -1, "Error: Event hash map get event %p failed.", evt);

    // not capture stream
    if (event_status.ptr != NULL) {
        // update capture status by event
        rc = hashmap_put(stream_map, (void *)stm, event_status.ptr, true);
        CHECK_COND_RETURN(rc == -1, "Failed to put stream %p to the hash map.", (void *)stm);
        LOG_DEBUG("Stream %p capture state set to: true, because of event.", (void *)stm);
    }
}

void set_event_create_status(void *evt)
{
    int rc = hashmap_put(event_map, evt, NULL, false);
    CHECK_COND_RETURN(rc == -1, "Error: Event hash map put event %p failed.", evt);
}

void set_event_record_status(void *evt, rtStream_t stm)
{
    MapValue event_status;
    int rc = hashmap_get(event_map, evt, &event_status);
    CHECK_COND_RETURN(rc == -1, "Error: Event hash map get event %p failed.", evt);
    void *head_stream = NULL;
    rc = hashmap_get_ptr(stream_map, (void *)stm, &head_stream);
    CHECK_COND_RETURN(rc == -1, "Failed to get stream %p ptr from the hash map.", (void *)stm);
    // capture
    if (head_stream != NULL) {
        rc = hashmap_put(event_map, evt, head_stream, true);
        CHECK_COND_RETURN(rc == -1, "Error: Event hash map put event %p failed.", evt);
        LOG_DEBUG("Event %p capture status is updated to true in recording.", evt);
    }
}

void remove_stream(void *unused, rtStream_t stm)
{
    (void)unused;
    LOG_DEBUG("Remove stream %p", stm);
    for (int i = 0; i < g_cache_streams.num_streams; ++i) {
        if (stm == g_cache_streams.streams[i]) {
            for (int j = i + 1; j < g_cache_streams.num_streams; ++j) {
                g_cache_streams.streams[j - 1] = g_cache_streams.streams[j];
            }
            g_cache_streams.num_streams -= 1;
            hashmap_remove(stream_map, (void *)stm);
            LOG_DEBUG("Stream position %d removed.", i);
            break;
        }
    }
}

void set_event_destroy_status(void *evt)
{
    (void)hashmap_remove(event_map, evt);
}