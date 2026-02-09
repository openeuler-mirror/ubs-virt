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
#ifndef __CORE_LIMITER_H__
#define __CORE_LIMITER_H__

#if defined(__cplusplus)
#include <atomic>
using atomic_int = std::atomic<int>;
#else
#include <stdatomic.h>
#endif

#include <acl/acl.h>
#include <runtime/rt.h>
#include <inttypes.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define MAGIC_INITIALIZED 0x495A4544U // IZED
#define MAGIC_INITIALIZING 0x5A494E47U // ZING
#define MAGIC_UNINITIALIZED 0x0
#define MAX_STREAMS_PER_PROCESS 128
#define MAX_EVENT_PER_PROCESS 65000
#define HUNDRED_PERCENT 100
#define NS_PER_US 1000ULL
#define NS_PER_MS 1000000ULL
#define NS_PER_S  1000000000ULL

#define VNPU_SCHEULE_PERIOD             (400ULL * NS_PER_MS) // 400 ms
#define VNPU_FLUSH_PERIOD               (1ULL * NS_PER_MS)   // 1ms
#define VNPU_TIMEOUT_PERIOD             (3ULL * NS_PER_MS)   // 3ms
#define VNPU_NO_TASK_TIMEOUT_PERIOD     (5ULL * NS_PER_MS)   // 5ms
#define WAITING_SLEEP_PERIOD            (100ULL * NS_PER_US)  // 100 us

typedef struct cache_streams {
    int num_streams;
    rtStream_t streams[MAX_STREAMS_PER_PROCESS];
} cache_streams_t;

typedef void (*core_function)(void* param, rtStream_t stream);

extern pthread_mutex_t g_sched_mutex;
extern atomic_int hasModelExecuteSync;
extern atomic_int waitEventCount;
extern int aicore_limiter_initialize(void);
extern void core_limiter(rtStream_t stream, core_function func, void* param);
extern void set_stream_capture(rtStream_t stream, void* capture);
extern void set_event_create_status(rtEvent_t evt);
extern void set_event_wait_status(void* evt, rtStream_t stm);
extern void set_event_record_status(rtEvent_t evt, rtStream_t stm);
extern void remove_stream(rtEvent_t evt, rtStream_t stm);
extern void set_event_destroy_status(rtEvent_t evt);
uint64_t ns_now(void);

#if defined(__cplusplus)
}
#endif

#endif // CORE_LIMITER_H