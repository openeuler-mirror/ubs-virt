/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 */

#ifndef __MEMORY_TRACKER_H__
#define __MEMORY_TRACKER_H__

#include <acl/acl.h>
#include <pthread.h>
#include <runtime/rt.h>
#include <stdbool.h>
#include <stdint.h>
#include "common.h"
#include "hash_map.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct MapRecordNode {
    void *va;
    rtDrvMemHandle handle;
    size_t size;
    size_t offset;
    uint64_t flags;
    struct MapRecordNode *next;
} MapRecordNode;

typedef struct HandleRecordNode {
    rtDrvMemHandle handle;
    size_t size;
    rtDrvMemProp_t prop;
    uint64_t flags;
    struct HandleRecordNode *next;
} HandleRecordNode;

typedef struct memory_record {
    void *ptr;
    uint64_t size;
    rtDrvMemHandle handle;
    bool is_swapped;
    bool is_physical;
    uint64_t swap_offset;
    uint64_t alloc_time_ns;
} memory_record_t;

typedef struct memory_tracker {
    pthread_mutex_t lock; /* RECURSIVE 锁：保护 records/map/count 的并发访问 */
    HashMap *map;
    memory_record_t *records;
    size_t capacity;
    size_t count;
    size_t check_record;
    size_t check_record_in;
    MapRecordNode *map_record_head;
    HandleRecordNode *handle_record_head;
} memory_tracker_t;

memory_tracker_t *memory_tracker_create(size_t capacity);
int memory_tracker_destroy(memory_tracker_t *tracker);
int memory_tracker_expand(memory_tracker_t *tracker, size_t new_capacity);

int memory_tracker_add(memory_tracker_t *tracker, void *ptr, uint64_t size, rtDrvMemHandle handle, bool is_physical);
int memory_tracker_remove(memory_tracker_t *tracker, void *ptr);
memory_record_t *memory_tracker_find(memory_tracker_t *tracker, void *ptr);

int memory_tracker_iterate(memory_tracker_t *tracker, void (*callback)(memory_record_t *record, void *ctx), void *ctx);

int memory_tracker_mark_swapped(memory_tracker_t *tracker, void *ptr, uint64_t offset);
int memory_tracker_mark_active(memory_tracker_t *tracker, void *ptr, rtDrvMemHandle new_handle);

int memory_tracker_collect_records(memory_tracker_t *tracker, memory_record_t ***records, int *count);
memory_record_t *memory_tracker_find_swapped(memory_tracker_t *tracker);
int memory_tracker_record_mark_swapped(memory_record_t *record, uint64_t offset);
int memory_record_mark_swapped(memory_record_t *record, uint64_t offset);

uint64_t get_record_size(memory_tracker_t *tracker, void *ptr, uint64_t size);
int memory_tracker_add_map_record(memory_tracker_t *tracker, void *ptr, size_t size, size_t offset,
                                  rtDrvMemHandle handle, uint64_t flags);
int memory_tracker_add_handle_record(memory_tracker_t *tracker, rtDrvMemHandle handle, size_t size, rtDrvMemProp_t prop,
                                     uint64_t flags);
int memory_tracker_remove_map_record(memory_tracker_t *tracker, void *ptr);
int memory_tracker_remove_handle_record(memory_tracker_t *tracker, rtDrvMemHandle handle);
int memory_tracker_get_map_va(memory_tracker_t *tracker, rtDrvMemHandle handle, void **va);
int memory_tracker_get_map_prop(memory_tracker_t *tracker, void *va, size_t *size, size_t *offset, uint64_t *flags);
int memory_tracker_get_handle_prop(memory_tracker_t *tracker, rtDrvMemHandle handle, size_t *size, rtDrvMemProp_t *prop,
                                   uint64_t *flags);

#if defined(__cplusplus)
}
#endif

#endif