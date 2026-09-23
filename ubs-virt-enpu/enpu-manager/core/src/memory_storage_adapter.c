/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 */

#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "securec.h"
#include "storage_adapter.h"

#define MEMORY_ADAPTER_MAX_CAPACITY 256

typedef struct memory_adapter_data {
    allocation_t *allocations;
    int count;
    int capacity;
} memory_adapter_data_t;

static int memory_adapter_save(storage_adapter_t *adapter, allocation_t **allocs, int count)
{
    memory_adapter_data_t *data = (memory_adapter_data_t *)adapter->private_data;
    if (!data) {
        return ENPU_FAIL;
    }

    if (count > data->capacity) {
        int new_capacity = count > MEMORY_ADAPTER_MAX_CAPACITY ? MEMORY_ADAPTER_MAX_CAPACITY : count;
        size_t new_size = (size_t)new_capacity * sizeof(allocation_t);
        allocation_t *new_allocs = (allocation_t *)malloc(new_size);
        if (new_allocs == NULL) {
            return ENPU_NO_RESOURCE;
        }
        size_t old_size = (size_t)data->count * sizeof(allocation_t);
        if (memcpy_s(new_allocs, new_size, data->allocations, old_size) != 0) {
            free(new_allocs);
            return ENPU_FAIL;
        }
        free(data->allocations);
        data->allocations = new_allocs;
        data->capacity = new_capacity;
    }

    data->count = 0;
    for (int i = 0; i < count && i < data->capacity; i++) {
        if (memcpy_s(&data->allocations[i], sizeof(allocation_t), allocs[i], sizeof(allocation_t)) != 0) {
            return ENPU_FAIL;
        }
        data->count++;
    }

    return ENPU_SUCCESS;
}

static int memory_adapter_load(storage_adapter_t *adapter, allocation_t *allocs, int *count)
{
    memory_adapter_data_t *data = (memory_adapter_data_t *)adapter->private_data;
    if (!data) {
        return ENPU_FAIL;
    }

    *count = data->count;
    if (data->count > 0 && allocs) {
        if (memcpy_s(allocs, data->count * sizeof(allocation_t), data->allocations,
                     data->count * sizeof(allocation_t)) != 0) {
            return ENPU_FAIL;
        }
    }

    return ENPU_SUCCESS;
}

static int memory_adapter_destroy(storage_adapter_t *adapter)
{
    memory_adapter_data_t *data = (memory_adapter_data_t *)adapter->private_data;
    if (data) {
        if (data->allocations) {
            free(data->allocations);
        }
        free(data);
    }
    free(adapter);
    return ENPU_SUCCESS;
}

storage_adapter_t *memory_storage_adapter_create(void)
{
    storage_adapter_t *adapter = calloc(1, sizeof(storage_adapter_t));
    if (!adapter) {
        return NULL;
    }

    memory_adapter_data_t *data = calloc(1, sizeof(memory_adapter_data_t));
    if (!data) {
        free(adapter);
        return NULL;
    }

    data->allocations = calloc(MEMORY_ADAPTER_MAX_CAPACITY, sizeof(allocation_t));
    if (!data->allocations) {
        free(data);
        free(adapter);
        return NULL;
    }

    data->capacity = MEMORY_ADAPTER_MAX_CAPACITY;
    data->count = 0;

    adapter->save = memory_adapter_save;
    adapter->load = memory_adapter_load;
    adapter->destroy = memory_adapter_destroy;
    adapter->private_data = data;

    return adapter;
}