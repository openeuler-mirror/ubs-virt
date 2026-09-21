/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 */

#include "memory_tracker.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "securec.h"

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

    memory_record_t *record = &tracker->records[tracker->count];
    record->ptr = ptr;
    record->size = size;
    record->handle = handle;
    record->is_swapped = false;
    record->is_physical = is_physical;
    record->swap_offset = 0;
    record->alloc_time_ns = get_time_ns();

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

    for (size_t i = found_index; i < tracker->count - 1; i++) {
        tracker->records[i] = tracker->records[i + 1];
        int ret = hashmap_put(tracker->map, tracker->records[i].ptr, &tracker->records[i], false);
        if (ret != ENPU_SUCCESS) {
            LOG_ERROR("Failed to move records, record index=%u.", i);
            pthread_mutex_unlock(&tracker->lock);
            return ENPU_FAIL;
        }
    }

    int ret = hashmap_remove(tracker->map, ptr);
    if (ret != 0) {
        pthread_mutex_unlock(&tracker->lock);
        return ENPU_FAIL;
    }

    tracker->count--;

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
                                  rtDrvMemHandle handle, uint64_t flags)
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

int memory_tracker_get_map_va(memory_tracker_t *tracker, rtDrvMemHandle handle, void **va)
{
    if (tracker == NULL) {
        return ENPU_FAIL;
    }
    pthread_mutex_lock(&tracker->lock);
    MapRecordNode *record = tracker->map_record_head;
    while (record->next) {
        record = record->next;
        if (record->handle == handle) {
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