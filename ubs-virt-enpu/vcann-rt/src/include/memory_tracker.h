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
    uint64_t gen; /* 所属内存记录的分配代次，用于隔离 handle 值复用 */
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
    /* 分配代次: 每次记录创建时全局单调递增。驱动在 rtFreePhysical 后会复用同一 handle 值,
     * 仅按 handle 匹配会命中上一代的 map record, 造成换出/换入串块 */
    uint64_t gen;
    /* 已换入成功但对应字节仍累计在 pending_restore_bytes 中(尚未加回 hbm_used):
     * 部分换入失败时该标志保留, 期间释放该记录只需核销 pending, 不能再扣一次 hbm_used */
    bool restore_pending;
} memory_record_t;

typedef struct memory_tracker {
    pthread_mutex_t lock; /* RECURSIVE 锁：保护 records/map/count 的并发访问 */
    HashMap *map;
    memory_record_t *records;
    size_t capacity;
    size_t count;
    size_t check_record;
    size_t check_record_in;
    /* 已成功换入但尚未加回 hbm_used 的字节数: 部分换入失败时保留, 下次重试成功后
     * 一次性恢复, 避免依赖可能被 check_and_swap_out 改写的 swap_size */
    uint64_t pending_restore_bytes;
    MapRecordNode *map_record_head;
    HandleRecordNode *handle_record_head;
} memory_tracker_t;

memory_tracker_t *memory_tracker_create(size_t capacity);
int memory_tracker_destroy(memory_tracker_t *tracker);
int memory_tracker_expand(memory_tracker_t *tracker, size_t new_capacity);

int memory_tracker_add(memory_tracker_t *tracker, void *ptr, uint64_t size, rtDrvMemHandle handle, bool is_physical);
int memory_tracker_remove(memory_tracker_t *tracker, void *ptr);
memory_record_t *memory_tracker_find(memory_tracker_t *tracker, void *ptr);
memory_record_t *memory_tracker_find_containing(memory_tracker_t *tracker, void *ptr);

int memory_tracker_iterate(memory_tracker_t *tracker, void (*callback)(memory_record_t *record, void *ctx), void *ctx);

int memory_tracker_mark_swapped(memory_tracker_t *tracker, void *ptr, uint64_t offset);
int memory_tracker_mark_active(memory_tracker_t *tracker, void *ptr, rtDrvMemHandle new_handle);
/* 按记录本身标记为活跃: handle/VA 被驱动复用时 ptr 不再唯一, 必须避免按 ptr 反查 */
int memory_record_mark_active(memory_record_t *record, rtDrvMemHandle new_handle);
/* 原子摘除: 在 tracker 锁内完成查找、字段拷贝与删除, 返回记录快照。
 * 调用方不得在锁外持有 records[] 元素指针: 并发删除会压缩数组、扩容会迁移数组,
 * 元素地址既不能作为删除键, 锁外读取的字段也会张冠李戴。 */
int memory_tracker_take(memory_tracker_t *tracker, void *ptr, memory_record_t *record, bool *found);

int memory_tracker_collect_records(memory_tracker_t *tracker, memory_record_t ***records, int *count);
memory_record_t *memory_tracker_find_swapped(memory_tracker_t *tracker);
int memory_tracker_record_mark_swapped(memory_record_t *record, uint64_t offset);
int memory_record_mark_swapped(memory_record_t *record, uint64_t offset);

uint64_t get_record_size(memory_tracker_t *tracker, void *ptr, uint64_t size);
int memory_tracker_add_map_record(memory_tracker_t *tracker, void *ptr, size_t size, size_t offset,
                                  rtDrvMemHandle handle, uint64_t flags, uint64_t gen);
int memory_tracker_add_handle_record(memory_tracker_t *tracker, rtDrvMemHandle handle, size_t size, rtDrvMemProp_t prop,
                                     uint64_t flags);
int memory_tracker_remove_map_record(memory_tracker_t *tracker, void *ptr);
int memory_tracker_remove_handle_record(memory_tracker_t *tracker, rtDrvMemHandle handle);
int memory_tracker_get_map_va(memory_tracker_t *tracker, rtDrvMemHandle handle, uint64_t gen, void **va);
int memory_tracker_get_map_prop(memory_tracker_t *tracker, void *va, size_t *size, size_t *offset, uint64_t *flags);
int memory_tracker_get_handle_prop(memory_tracker_t *tracker, rtDrvMemHandle handle, size_t *size, rtDrvMemProp_t *prop,
                                   uint64_t *flags);

#if defined(__cplusplus)
}
#endif

#endif