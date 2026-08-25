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
#ifndef __VNPU_STATS_H__
#define __VNPU_STATS_H__

#include <acl/acl.h>
#include <runtime/rt.h>
#include <stdint.h>
#include "npu_manager.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define VNPU_STATS_NS_PER_MS 1000000ULL
#define VNPU_STATS_NS_PER_US 1000ULL

#define VNPU_STATS_BUCKET_DURATION_NS (50ULL * VNPU_STATS_NS_PER_MS)
#define VNPU_STATS_NUM_BUCKETS 12
#define VNPU_STATS_WINDOW_NS (1000ULL * VNPU_STATS_NS_PER_MS)
#define VNPU_STATS_INIT_TIMEOUT_NS (3ULL * VNPU_STATS_NS_PER_MS)
#define VNPU_STATS_WAIT_SLEEP_NS (100ULL * VNPU_STATS_NS_PER_US)
#define VNPU_STATS_AVG_STALE_NS (3ULL * VNPU_STATS_WINDOW_NS)
#define INVALID_SAMPLE_PERIOD VNPU_STATS_WINDOW_NS
#define DEFAULT_KERNEL_AVE_EXE_TIME (1ULL * VNPU_STATS_NS_PER_MS)
#define VNPU_STATS_AVG_SLOTS 64
#define VNPU_STATS_SHM_ID_SUFFIX "_stats"

#define VNPU_STATS_MAGIC_UNINITIALIZED 0x0U
#define VNPU_STATS_MAGIC_INITIALIZING 0x5A494E47U /* "ZING" */
#define VNPU_STATS_MAGIC_INITIALIZED 0x495A4546U  /* "IZEF" (v2, was 0x495A4544 "IZED") */

typedef struct {
    atomic_uint_fast64_t bucket_start_ns;
    atomic_uint_fast64_t sum_block_dim;
    atomic_uint_fast64_t launch_count;
} vnpu_stats_bucket_t;

typedef struct {
    atomic_uint_fast64_t sum_ns;
    atomic_uint_fast64_t count;
    atomic_uint_fast64_t ts_ns;
} vnpu_stats_avg_slot_t;

typedef struct {
    vnpu_stats_bucket_t buckets[MAX_VNPU][VNPU_STATS_NUM_BUCKETS];
    atomic_uint_fast64_t avg_slot_seq[MAX_VNPU];
    vnpu_stats_avg_slot_t avg_slots[MAX_VNPU][VNPU_STATS_AVG_SLOTS];
    atomic_uint_fast32_t magic_number;
} vnpu_stats_shm_t;

typedef struct {
    uint64_t sum_block_dim;
    uint64_t launch_count;
} vnpu_stats_aggregate_t;

typedef struct SampleRecordNode {
    int kernel_count;
    uint64_t sample_time;
    uint64_t timestamp;
    struct SampleRecordNode *next;
} SampleRecordNode;

extern int vnpu_stats_init(const char *base_shm_id);
extern void vnpu_stats_record(uint8_t vnpu_id, uint32_t block_dim, uint64_t count);
extern int vnpu_stats_query(uint8_t vnpu_id, vnpu_stats_aggregate_t *out);
extern void vnpu_stats_set_local_avg_duration(uint8_t vnpu_id, uint64_t avg_duration_ns, uint64_t count);
extern int64_t vnpu_stats_get_avg_duration_ns(uint8_t vnpu_id);
extern int get_random_fd(void);
extern bool is_random_sampling(void);
extern void sampling_begin(rtStream_t stm);
extern void record_event_time(rtStream_t stm, int count);
extern void sampling_end(rtStream_t stm);
extern int add_sample_record(int kernel_count, uint64_t sample_time, uint64_t timestamp);
extern void remove_sample_records(SampleRecordNode *node);
extern uint64_t get_ker_ave_exec_time(uint64_t *count);
extern int is_stream_capture(rtStream_t stm, rtStreamCaptureStatus *status);
extern int get_kernel_count(rtModel_t mdl);
extern void sampling_graph_end(rtStream_t stm, rtModel_t mdl);
extern void *sample_sync(void *args);

#if defined(__cplusplus)
}
#endif

#endif /* __VNPU_STATS_H__ */
