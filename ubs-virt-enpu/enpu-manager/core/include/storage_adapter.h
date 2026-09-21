/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 */

#ifndef __STORAGE_ADAPTER_H__
#define __STORAGE_ADAPTER_H__

#include "allocation.h"
#include "common.h"

typedef struct storage_adapter storage_adapter_t;

typedef int (*storage_save_func)(storage_adapter_t *adapter, allocation_t **allocs, int count);
typedef int (*storage_load_func)(storage_adapter_t *adapter, allocation_t *allocs, int *count);
typedef int (*storage_destroy_func)(storage_adapter_t *adapter);

struct storage_adapter {
    storage_save_func save;
    storage_load_func load;
    storage_destroy_func destroy;
    void *private_data;
};

storage_adapter_t *memory_storage_adapter_create(void);
storage_adapter_t *file_storage_adapter_create(const char *path);

int storage_adapter_save(storage_adapter_t *adapter, allocation_t **allocs, int count);
int storage_adapter_load(storage_adapter_t *adapter, allocation_t *allocs, int *count);
int storage_adapter_destroy(storage_adapter_t *adapter);

#endif