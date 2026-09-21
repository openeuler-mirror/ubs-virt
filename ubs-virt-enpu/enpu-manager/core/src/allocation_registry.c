/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "allocation.h"
#include "securec.h"
#include "storage_adapter.h"

#define ALLOC_REGISTRY_INIT_CAPACITY 64

struct allocation_registry {
    allocation_t *entries;
    int count;
    int capacity;
    pthread_mutex_t lock;
    storage_adapter_t *adapter;
};

allocation_registry_t *allocation_registry_create(void)
{
    return allocation_registry_create_with_adapter(NULL);
}

allocation_registry_t *allocation_registry_create_with_adapter(storage_adapter_t *adapter)
{
    allocation_registry_t *registry = calloc(1, sizeof(allocation_registry_t));
    if (!registry) {
        return NULL;
    }

    registry->entries = calloc(ALLOC_REGISTRY_INIT_CAPACITY, sizeof(allocation_t));
    if (!registry->entries) {
        free(registry);
        return NULL;
    }

    registry->capacity = ALLOC_REGISTRY_INIT_CAPACITY;
    registry->count = 0;
    registry->adapter = adapter;
    pthread_mutex_init(&registry->lock, NULL);

    return registry;
}

int allocation_registry_destroy(allocation_registry_t *registry)
{
    if (!registry) {
        return ENPU_INVALID_PARAM;
    }

    pthread_mutex_destroy(&registry->lock);
    free(registry->entries);
    free(registry);

    return ENPU_SUCCESS;
}

static int find_entry_index(allocation_registry_t *registry, const char *pod_uid, const char *container_name)
{
    for (int i = 0; i < registry->count; i++) {
        if (strcmp(registry->entries[i].pod_uid, pod_uid) == 0 &&
            strcmp(registry->entries[i].container_name, container_name) == 0) {
            return i;
        }
    }
    return -1;
}

static int persist_to_adapter(allocation_registry_t *registry)
{
    if (!registry->adapter) {
        return ENPU_SUCCESS;
    }

    allocation_t *allocs[MAX_ALLOC_COUNT] = {0};
    for (int i = 0; i < registry->count; i++) {
        allocs[i] = &registry->entries[i];
    }

    return storage_adapter_save(registry->adapter, allocs, registry->count);
}

int allocation_registry_register(allocation_registry_t *registry, const allocation_t *allocation)
{
    if (!registry || !allocation) {
        return ENPU_INVALID_PARAM;
    }
    if (allocation->pod_uid[0] == '\0' || allocation->container_name[0] == '\0') {
        return ENPU_INVALID_PARAM;
    }

    pthread_mutex_lock(&registry->lock);

    int idx = find_entry_index(registry, allocation->pod_uid, allocation->container_name);
    if (idx >= 0) {
        pthread_mutex_unlock(&registry->lock);
        return ENPU_ALREADY_EXISTS;
    }

    if (registry->count >= MAX_ALLOC_COUNT) {
        pthread_mutex_unlock(&registry->lock);
        return ENPU_NO_RESOURCE;
    }

    if (registry->count >= registry->capacity) {
        int new_capacity = registry->capacity * 2;
        if (new_capacity > MAX_ALLOC_COUNT) {
            new_capacity = MAX_ALLOC_COUNT;
        }
        allocation_t *new_entries = (allocation_t *)malloc((size_t)new_capacity * sizeof(allocation_t));
        if (!new_entries) {
            pthread_mutex_unlock(&registry->lock);
            return ENPU_NO_RESOURCE;
        }
        memcpy_s(new_entries, (size_t)new_capacity * sizeof(allocation_t), registry->entries,
                 (size_t)registry->count * sizeof(allocation_t));
        free(registry->entries);
        registry->entries = new_entries;
        registry->capacity = new_capacity;
    }

    memcpy_s(&registry->entries[registry->count], sizeof(allocation_t), allocation, sizeof(allocation_t));
    registry->count++;

    int ret = persist_to_adapter(registry);

    pthread_mutex_unlock(&registry->lock);
    return ret;
}

int allocation_registry_release(allocation_registry_t *registry, const char *pod_uid, const char *container_name)
{
    if (!registry || !pod_uid || !container_name) {
        return ENPU_INVALID_PARAM;
    }

    pthread_mutex_lock(&registry->lock);

    int idx = find_entry_index(registry, pod_uid, container_name);
    if (idx < 0) {
        pthread_mutex_unlock(&registry->lock);
        return ENPU_NOT_FOUND;
    }

    if (idx < registry->count - 1) {
        size_t move_bytes = (size_t)(registry->count - idx - 1) * sizeof(allocation_t);
        memmove_s(&registry->entries[idx], move_bytes, &registry->entries[idx + 1], move_bytes);
    }
    registry->count--;

    int ret = persist_to_adapter(registry);

    pthread_mutex_unlock(&registry->lock);
    return ret;
}

int allocation_registry_update_shm_id(allocation_registry_t *registry, const char *pod_uid, const char *container_name,
                                      const char *new_shm_id)
{
    if (!registry || !pod_uid || !container_name || !new_shm_id) {
        return ENPU_INVALID_PARAM;
    }

    pthread_mutex_lock(&registry->lock);

    int idx = find_entry_index(registry, pod_uid, container_name);
    if (idx < 0) {
        pthread_mutex_unlock(&registry->lock);
        return ENPU_NOT_FOUND;
    }

    strncpy_s(registry->entries[idx].shm_id, DIE_ID_LEN, new_shm_id, DIE_ID_LEN - 1);

    int ret = persist_to_adapter(registry);

    pthread_mutex_unlock(&registry->lock);
    return ret;
}

int allocation_registry_get(allocation_registry_t *registry, const char *pod_uid, const char *container_name,
                            allocation_t *allocation)
{
    if (!registry || !pod_uid || !container_name || !allocation) {
        return ENPU_INVALID_PARAM;
    }

    pthread_mutex_lock(&registry->lock);

    int idx = find_entry_index(registry, pod_uid, container_name);
    if (idx < 0) {
        pthread_mutex_unlock(&registry->lock);
        return ENPU_NOT_FOUND;
    }

    memcpy_s(allocation, sizeof(allocation_t), &registry->entries[idx], sizeof(allocation_t));

    pthread_mutex_unlock(&registry->lock);
    return ENPU_SUCCESS;
}

int allocation_registry_list(allocation_registry_t *registry, allocation_t *allocations, int *count)
{
    if (!registry || !allocations || !count) {
        return ENPU_INVALID_PARAM;
    }

    pthread_mutex_lock(&registry->lock);

    *count = registry->count;
    if (registry->count > 0) {
        memcpy_s(allocations, (size_t)registry->count * sizeof(allocation_t), registry->entries,
                 (size_t)registry->count * sizeof(allocation_t));
    }

    pthread_mutex_unlock(&registry->lock);
    return ENPU_SUCCESS;
}

int allocation_registry_get_by_phy_id_vnpu_id(allocation_registry_t *registry, int32_t phy_id, int32_t vnpu_id,
                                              allocation_t *allocation)
{
    if (!registry || !allocation) {
        return ENPU_INVALID_PARAM;
    }

    pthread_mutex_lock(&registry->lock);

    for (int i = 0; i < registry->count; i++) {
        if (registry->entries[i].phy_id == phy_id && registry->entries[i].vnpu_id == vnpu_id) {
            memcpy_s(allocation, sizeof(allocation_t), &registry->entries[i], sizeof(allocation_t));
            pthread_mutex_unlock(&registry->lock);
            return ENPU_SUCCESS;
        }
    }

    pthread_mutex_unlock(&registry->lock);
    return ENPU_NOT_FOUND;
}

int allocation_registry_list_by_phy_id(allocation_registry_t *registry, int32_t phy_id, allocation_t *allocations,
                                       int *count)
{
    if (!registry || !allocations || !count) {
        return ENPU_INVALID_PARAM;
    }

    pthread_mutex_lock(&registry->lock);

    int found = 0;
    for (int i = 0; i < registry->count; i++) {
        if (registry->entries[i].phy_id == phy_id) {
            memcpy_s(&allocations[found], sizeof(allocation_t), &registry->entries[i], sizeof(allocation_t));
            found++;
        }
    }

    *count = found;
    pthread_mutex_unlock(&registry->lock);
    return ENPU_SUCCESS;
}

int allocation_registry_save(allocation_registry_t *registry)
{
    if (!registry) {
        return ENPU_INVALID_PARAM;
    }

    pthread_mutex_lock(&registry->lock);
    int ret = persist_to_adapter(registry);
    pthread_mutex_unlock(&registry->lock);

    return ret;
}

int allocation_registry_recover(allocation_registry_t *registry)
{
    if (!registry || !registry->adapter) {
        return ENPU_INVALID_PARAM;
    }

    pthread_mutex_lock(&registry->lock);

    allocation_t loaded[MAX_ALLOC_COUNT] = {0};
    int count = 0;
    int ret = storage_adapter_load(registry->adapter, loaded, &count);
    if (ret != ENPU_SUCCESS) {
        pthread_mutex_unlock(&registry->lock);
        return ret;
    }

    if (count > registry->capacity) {
        int new_capacity = count;
        allocation_t *new_entries = (allocation_t *)malloc((size_t)new_capacity * sizeof(allocation_t));
        if (!new_entries) {
            pthread_mutex_unlock(&registry->lock);
            return ENPU_NO_RESOURCE;
        }
        memcpy_s(new_entries, (size_t)new_capacity * sizeof(allocation_t), registry->entries,
                 (size_t)registry->count * sizeof(allocation_t));
        free(registry->entries);
        registry->entries = new_entries;
        registry->capacity = new_capacity;
    }

    memcpy_s(registry->entries, (size_t)registry->capacity * sizeof(allocation_t), loaded,
             (size_t)count * sizeof(allocation_t));
    registry->count = count;

    pthread_mutex_unlock(&registry->lock);
    return ENPU_SUCCESS;
}