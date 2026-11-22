/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 */

#include "memory_tracker.h"
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "securec.h"

/* 进程内全局分配代次: 隔离驱动复用同一 handle 值产生的跨代记录 */
static atomic_uint_fast64_t g_alloc_gen = 0;

static uint64_t get_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

memory_tracker_t *memory_tracker_create(size_t capacity)
{
    if (capacity == 0) {
        return NULL;
    }
    if (capacity > SIZE_MAX / sizeof(memory_record_t)) {
        return NULL;
    }

    memory_tracker_t *tracker = (memory_tracker_t *)calloc(1, sizeof(memory_tracker_t));
    if (tracker == NULL) {
        return NULL;
    }

    tracker->records = (memory_record_t *)calloc(capacity, sizeof(memory_record_t));
    if (tracker->records == NULL) {
        free(tracker);
        return NULL;
    }

    tracker->map = hashmap_create(capacity);
    if (tracker->map == NULL) {
        free(tracker->records);
        free(tracker);
        return NULL;
    }

    tracker->capacity = capacity;
    tracker->count = 0;

    tracker->map_record_head = (MapRecordNode *)malloc(sizeof(MapRecordNode));
    if (!tracker->map_record_head) {
        LOG_ERROR("Map record malloc head node failed.");
        free(tracker->records);
        free(tracker);
        return NULL;
    }
    tracker->map_record_head->next = NULL;

    tracker->handle_record_head = (HandleRecordNode *)malloc(sizeof(HandleRecordNode));
    if (!tracker->handle_record_head) {
        LOG_ERROR("Handle record malloc head node failed.");
        free(tracker->map_record_head);
        free(tracker->records);
        free(tracker);
        return NULL;
    }
    tracker->handle_record_head->next = NULL;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&tracker->lock, &attr);
    pthread_mutexattr_destroy(&attr);

    return tracker;
}

int memory_tracker_destroy(memory_tracker_t *tracker)
{
    if (tracker == NULL) {
        return ENPU_FAIL;
    }

    pthread_mutex_destroy(&tracker->lock);
    hashmap_destroy(tracker->map);
    free(tracker->records);
    free(tracker);

    return ENPU_SUCCESS;
}

int memory_tracker_expand(memory_tracker_t *tracker, size_t new_capacity)
{
    if (new_capacity == 0 || new_capacity > SIZE_MAX / sizeof(memory_record_t)) {
        return ENPU_FAIL;
    }

    /* 禁用 realloc: 分配新块 + 拷贝旧数据 + 释放旧块, 行为可控 */
    memory_record_t *new_records = (memory_record_t *)calloc(new_capacity, sizeof(memory_record_t));
    if (new_records == NULL) {
        return ENPU_FAIL;
    }
    if (tracker->records != NULL && tracker->count > 0) {
        size_t copy_count = tracker->count < new_capacity ? tracker->count : new_capacity;
        int copy_ret = memcpy_s(new_records, new_capacity * sizeof(memory_record_t), tracker->records,
                                copy_count * sizeof(memory_record_t));
        if (copy_ret != 0) {
            free(new_records);
            return ENPU_FAIL;
        }
    }

    HashMap *new_map = hashmap_create(new_capacity);
    if (new_map == NULL) {
        free(new_records);
        return ENPU_FAIL;
    }

    for (size_t i = 0; i < tracker->count; i++) {
        void *key = new_records[i].ptr;
        if (key != NULL) {
            int ret = hashmap_put(new_map, key, &new_records[i], false);
            CHECK_RETURN_ERROR_CODE(ret, "Failed to move records to new map.");
        }
    }

    hashmap_destroy(tracker->map);
    tracker->map = new_map;
    free(tracker->records);
    tracker->records = new_records;
    tracker->capacity = new_capacity;
    return ENPU_SUCCESS;
}

int memory_tracker_add(memory_tracker_t *tracker, void *ptr, uint64_t size, rtDrvMemHandle handle, bool is_physical)
{
    if (tracker == NULL || ptr == NULL || size == 0) {
        return ENPU_FAIL;
    }

    pthread_mutex_lock(&tracker->lock);

    if (tracker->count >= tracker->capacity) {
        pthread_mutex_unlock(&tracker->lock);
        return ENPU_FAIL;
    }

    /* 驱动释放后可能复用同一 handle/VA: 若映射里仍有旧记录(典型的僵尸记录),
     * 先丢弃它, 避免 records[] 中出现两条同 ptr 记录, 使后续 mark_active/remove 命中错误的那条 */
    void *stale_value = NULL;
    if (hashmap_get_ptr(tracker->map, ptr, &stale_value) == 0 && stale_value != NULL) {
        memory_record_t *stale = (memory_record_t *)stale_value;
        LOG_WARN("memory_tracker_add: ptr=%p reused by gen=%lu, drop stale record gen=%lu, swapped=%d, offset=%lu", ptr,
                 atomic_load(&g_alloc_gen), stale->gen, (int)stale->is_swapped, stale->swap_offset);
        (void)memory_tracker_remove(tracker, ptr);
    }

    memory_record_t *record = &tracker->records[tracker->count];
    record->ptr = ptr;
    record->size = size;
    record->handle = handle;
    record->is_swapped = false;
    record->is_physical = is_physical;
    record->swap_offset = 0;
    record->alloc_time_ns = get_time_ns();
    record->gen = atomic_fetch_add(&g_alloc_gen, 1);
    record->restore_pending = false;

    int ret = hashmap_put(tracker->map, ptr, record, false);
    if (ret != 0) {
        pthread_mutex_unlock(&tracker->lock);
        return ENPU_FAIL;
    }

    tracker->count++;
    if (tracker->capacity <= 2 * tracker->count) {
        size_t new_capacity = tracker->capacity * 2;
        ret = memory_tracker_expand(tracker, new_capacity);
        if (ret != ENPU_SUCCESS) {
            LOG_ERROR("Failed to expand memory tracker's records capacity.");
            pthread_mutex_unlock(&tracker->lock);
            return ENPU_FAIL;
        }
    }

    pthread_mutex_unlock(&tracker->lock);
    return ENPU_SUCCESS;
}

static void memory_tracker_cancel_pending_locked(memory_tracker_t *tracker, uint64_t size)
{
    uint64_t pending = tracker->pending_restore_bytes;
    tracker->pending_restore_bytes = size < pending ? pending - size : 0;
}

static int memory_tracker_remove_at_locked(memory_tracker_t *tracker, size_t index)
{
    void *ptr = tracker->records[index].ptr;

    /* 该记录换入后尚未把字节加回 hbm_used, 删除时同步核销 pending, 避免残留量在
     * 后续换入完成时被重复加回 */
    if (tracker->records[index].restore_pending) {
        memory_tracker_cancel_pending_locked(tracker, tracker->records[index].size);
    }

    for (size_t i = index; i < tracker->count - 1; i++) {
        tracker->records[i] = tracker->records[i + 1];
        int ret = hashmap_put(tracker->map, tracker->records[i].ptr, &tracker->records[i], false);
        if (ret != ENPU_SUCCESS) {
            LOG_ERROR("Failed to move records, record index=%u.", i);
            return ENPU_FAIL;
        }
    }

    tracker->count--;

    /* handle/VA 可能被驱动复用: 若 records[] 中仍有同 ptr 记录, 把 key 指回幸存记录;
     * 直接删 key 会让幸存记录变成 find() 查不到的孤儿, 而 swap 遍历仍会看到它 */
    for (size_t i = 0; i < tracker->count; i++) {
        if (tracker->records[i].ptr == ptr) {
            int ret = hashmap_put(tracker->map, ptr, &tracker->records[i], false);
            if (ret != ENPU_SUCCESS) {
                LOG_ERROR("Failed to rebind map key for surviving record, index=%u.", i);
                return ENPU_FAIL;
            }
            return ENPU_SUCCESS;
        }
    }

    int ret = hashmap_remove(tracker->map, ptr);
    return ret == 0 ? ENPU_SUCCESS : ENPU_FAIL;
}

int memory_tracker_remove(memory_tracker_t *tracker, void *ptr)
{
    if (tracker == NULL || ptr == NULL) {
        return ENPU_FAIL;
    }

    pthread_mutex_lock(&tracker->lock);

    int found_index = -1;
    for (size_t i = 0; i < tracker->count; i++) {
        if (tracker->records[i].ptr == ptr) {
            found_index = i;
            break;
        }
    }

    if (found_index == -1) {
        LOG_INFO("Record not found.");
        pthread_mutex_unlock(&tracker->lock);
        return ENPU_SUCCESS;
    }

    int ret = memory_tracker_remove_at_locked(tracker, (size_t)found_index);
    pthread_mutex_unlock(&tracker->lock);
    return ret;
}

int memory_tracker_take(memory_tracker_t *tracker, void *ptr, memory_record_t *record, bool *found)
{
    if (tracker == NULL || ptr == NULL || record == NULL || found == NULL) {
        return ENPU_FAIL;
    }

    pthread_mutex_lock(&tracker->lock);

    *found = false;
    for (size_t i = 0; i < tracker->count; i++) {
        if (tracker->records[i].ptr != ptr) {
            continue;
        }
        *record = tracker->records[i];
        *found = true;
        int ret = memory_tracker_remove_at_locked(tracker, i);
        pthread_mutex_unlock(&tracker->lock);
        return ret;
    }

    pthread_mutex_unlock(&tracker->lock);
    return ENPU_SUCCESS;
}

memory_record_t *memory_tracker_find(memory_tracker_t *tracker, void *ptr)
{
    if (tracker == NULL || ptr == NULL) {
        return NULL;
    }

    pthread_mutex_lock(&tracker->lock);

    void *value_ptr = NULL;
    int ret = hashmap_get_ptr(tracker->map, ptr, &value_ptr);
    if (ret != 0 || value_ptr == NULL) {
        pthread_mutex_unlock(&tracker->lock);
        return NULL;
    }

    memory_record_t *result = (memory_record_t *)value_ptr;
    pthread_mutex_unlock(&tracker->lock);
    return result;
}

memory_record_t *memory_tracker_find_containing(memory_tracker_t *tracker, void *ptr)
{
    if (tracker == NULL || ptr == NULL) {
        return NULL;
    }

    pthread_mutex_lock(&tracker->lock);

    void *value_ptr = NULL;
    int ret = hashmap_get_ptr(tracker->map, ptr, &value_ptr);
    if (ret == 0 && value_ptr != NULL) {
        pthread_mutex_unlock(&tracker->lock);
        return (memory_record_t *)value_ptr;
    }

    const uintptr_t addr = (uintptr_t)ptr;
    for (size_t i = 0; i < tracker->count; ++i) {
        memory_record_t *record = &tracker->records[i];
        if (record->is_physical || record->ptr == NULL) {
            continue;
        }
        const uintptr_t base = (uintptr_t)record->ptr;
        if (addr >= base && addr - base < record->size) {
            pthread_mutex_unlock(&tracker->lock);
            return record;
        }
    }

    pthread_mutex_unlock(&tracker->lock);
    return NULL;
}

int memory_tracker_iterate(memory_tracker_t *tracker, void (*callback)(memory_record_t *record, void *ctx), void *ctx)
{
    (void)tracker;
    (void)callback;
    (void)ctx;
    return ENPU_FAIL;
}

int memory_record_mark_swapped(memory_record_t *record, uint64_t offset)
{
    if (record == NULL) {
        return ENPU_FAIL;
    }

    record->is_swapped = true;
    record->swap_offset = offset;

    return ENPU_SUCCESS;
}

int memory_record_mark_active(memory_record_t *record, rtDrvMemHandle new_handle)
{
    if (record == NULL) {
        return ENPU_FAIL;
    }

    record->is_swapped = false;
    record->handle = new_handle;
    record->swap_offset = 0;

    return ENPU_SUCCESS;
}

int memory_tracker_mark_swapped(memory_tracker_t *tracker, void *ptr, uint64_t offset)
{
    if (tracker == NULL || ptr == NULL) {
        return ENPU_FAIL;
    }

    pthread_mutex_lock(&tracker->lock);

    memory_record_t *record = memory_tracker_find(tracker, ptr);
    if (record == NULL) {
        pthread_mutex_unlock(&tracker->lock);
        return ENPU_FAIL;
    }

    record->is_swapped = true;
    record->swap_offset = offset;

    pthread_mutex_unlock(&tracker->lock);
    return ENPU_SUCCESS;
}

int memory_tracker_mark_active(memory_tracker_t *tracker, void *ptr, rtDrvMemHandle new_handle)
{
    if (tracker == NULL || ptr == NULL) {
        return ENPU_FAIL;
    }

    pthread_mutex_lock(&tracker->lock);

    memory_record_t *record = memory_tracker_find(tracker, ptr);
    if (record == NULL) {
        pthread_mutex_unlock(&tracker->lock);
        return ENPU_FAIL;
    }

    record->is_swapped = false;
    record->handle = new_handle;
    record->swap_offset = 0;

    pthread_mutex_unlock(&tracker->lock);
    return ENPU_SUCCESS;
}

int memory_tracker_collect_records(memory_tracker_t *tracker, memory_record_t ***records, int *count)
{
    if (tracker == NULL || records == NULL || count == NULL) {
        return ENPU_FAIL;
    }

    pthread_mutex_lock(&tracker->lock);

    *records = (memory_record_t **)malloc(tracker->count * sizeof(memory_record_t *));
    if (*records == NULL) {
        pthread_mutex_unlock(&tracker->lock);
        return ENPU_FAIL;
    }

    for (size_t i = 0; i < tracker->count; i++) {
        (*records)[i] = &tracker->records[i];
    }

    *count = (int)tracker->count;

    pthread_mutex_unlock(&tracker->lock);
    return ENPU_SUCCESS;
}

memory_record_t *memory_tracker_find_swapped(memory_tracker_t *tracker)
{
    if (tracker == NULL) {
        return NULL;
    }

    pthread_mutex_lock(&tracker->lock);

    for (size_t i = 0; i < tracker->count; i++) {
        if (tracker->records[i].is_swapped) {
            memory_record_t *result = &tracker->records[i];
            pthread_mutex_unlock(&tracker->lock);
            return result;
        }
    }

    pthread_mutex_unlock(&tracker->lock);
    return NULL;
}

uint64_t get_record_size(memory_tracker_t *tracker, void *ptr, uint64_t size)
{
    if (tracker == NULL) {
        return size;
    }

    pthread_mutex_lock(&tracker->lock);

    memory_record_t *record = memory_tracker_find(tracker, ptr);
    if (record == NULL) {
        pthread_mutex_unlock(&tracker->lock);
        return size;
    }
    uint64_t result = record->size;
    pthread_mutex_unlock(&tracker->lock);
    return result;
}

int memory_tracker_add_map_record(memory_tracker_t *tracker, void *ptr, size_t size, size_t offset,
                                  rtDrvMemHandle handle, uint64_t flags, uint64_t gen)
{
    if (tracker == NULL) {
        return ENPU_FAIL;
    }
    pthread_mutex_lock(&tracker->lock);
    MapRecordNode *new_node = (MapRecordNode *)malloc(sizeof(MapRecordNode));
    if (!new_node) {
        LOG_ERROR("Add map record malloc node failed.");
        return ENPU_FAIL;
    }

    new_node->va = ptr;
    new_node->handle = handle;
    new_node->gen = gen;
    new_node->size = size;
    new_node->offset = offset;
    new_node->flags = flags;

    new_node->next = tracker->map_record_head->next;
    tracker->map_record_head->next = new_node;
    pthread_mutex_unlock(&tracker->lock);
    return ENPU_SUCCESS;
}

int memory_tracker_add_handle_record(memory_tracker_t *tracker, rtDrvMemHandle handle, size_t size, rtDrvMemProp_t prop,
                                     uint64_t flags)
{
    if (tracker == NULL) {
        return ENPU_FAIL;
    }
    pthread_mutex_lock(&tracker->lock);
    HandleRecordNode *new_node = (HandleRecordNode *)malloc(sizeof(HandleRecordNode));
    if (!new_node) {
        LOG_ERROR("Add map record malloc node failed.");
        return ENPU_FAIL;
    }
    new_node->handle = handle;
    new_node->size = size;
    new_node->prop = prop;
    new_node->flags = flags;

    new_node->next = tracker->handle_record_head->next;
    tracker->handle_record_head->next = new_node;
    pthread_mutex_unlock(&tracker->lock);
    return ENPU_SUCCESS;
}

int memory_tracker_remove_map_record(memory_tracker_t *tracker, void *ptr)
{
    if (tracker == NULL) {
        return ENPU_FAIL;
    }
    pthread_mutex_lock(&tracker->lock);
    MapRecordNode *record = tracker->map_record_head;
    while (record->next) {
        MapRecordNode *last_record = record;
        record = record->next;
        if (record->va == ptr) {
            last_record->next = record->next;
            free(record);
            record = last_record;
            pthread_mutex_unlock(&tracker->lock);
            break;
        }
    }
    pthread_mutex_unlock(&tracker->lock);
    return ENPU_SUCCESS;
}

int memory_tracker_remove_handle_record(memory_tracker_t *tracker, rtDrvMemHandle handle)
{
    if (tracker == NULL) {
        return ENPU_FAIL;
    }
    pthread_mutex_lock(&tracker->lock);
    HandleRecordNode *record = tracker->handle_record_head;
    while (record->next) {
        HandleRecordNode *last_record = record;
        record = record->next;
        if (record->handle == handle) {
            last_record->next = record->next;
            free(record);
            record = last_record;
            pthread_mutex_unlock(&tracker->lock);
            break;
        }
    }
    pthread_mutex_unlock(&tracker->lock);
    return ENPU_SUCCESS;
}

int memory_tracker_get_map_va(memory_tracker_t *tracker, rtDrvMemHandle handle, uint64_t gen, void **va)
{
    if (tracker == NULL) {
        return ENPU_FAIL;
    }
    pthread_mutex_lock(&tracker->lock);
    MapRecordNode *record = tracker->map_record_head;
    while (record->next) {
        record = record->next;
        /* (handle, gen) 双键匹配: 仅同代记录可命中, 避免复用 handle 值时串到上一代 VA */
        if (record->handle == handle && record->gen == gen) {
            *va = record->va;
            pthread_mutex_unlock(&tracker->lock);
            return ENPU_SUCCESS;
        }
    }
    *va = NULL;
    pthread_mutex_unlock(&tracker->lock);
    return ENPU_SUCCESS;
}

int memory_tracker_get_map_prop(memory_tracker_t *tracker, void *va, size_t *size, size_t *offset, uint64_t *flags)
{
    if (tracker == NULL) {
        return ENPU_FAIL;
    }
    pthread_mutex_lock(&tracker->lock);
    MapRecordNode *record = tracker->map_record_head;
    while (record->next) {
        record = record->next;
        if (record->va == va) {
            *size = record->size;
            *offset = record->offset;
            *flags = record->flags;
            pthread_mutex_unlock(&tracker->lock);
            return ENPU_SUCCESS;
        }
    }
    pthread_mutex_unlock(&tracker->lock);
    return ENPU_FAIL;
}

int memory_tracker_get_handle_prop(memory_tracker_t *tracker, rtDrvMemHandle handle, size_t *size, rtDrvMemProp_t *prop,
                                   uint64_t *flags)
{
    if (tracker == NULL) {
        return ENPU_FAIL;
    }
    pthread_mutex_lock(&tracker->lock);
    HandleRecordNode *record = tracker->handle_record_head;
    while (record->next) {
        record = record->next;
        if (record->handle == handle) {
            *size = record->size;
            *prop = record->prop;
            *flags = record->flags;
            pthread_mutex_unlock(&tracker->lock);
            return ENPU_SUCCESS;
        }
    }
    pthread_mutex_unlock(&tracker->lock);
    return ENPU_FAIL;
}