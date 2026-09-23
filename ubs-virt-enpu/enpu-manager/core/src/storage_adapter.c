/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 */

#include "storage_adapter.h"
#include <stdlib.h>
#include <string.h>

int storage_adapter_save(storage_adapter_t *adapter, allocation_t **allocs, int count)
{
    if (!adapter || !adapter->save) {
        return ENPU_INVALID_PARAM;
    }
    return adapter->save(adapter, allocs, count);
}

int storage_adapter_load(storage_adapter_t *adapter, allocation_t *allocs, int *count)
{
    if (!adapter || !adapter->load) {
        return ENPU_INVALID_PARAM;
    }
    return adapter->load(adapter, allocs, count);
}

int storage_adapter_destroy(storage_adapter_t *adapter)
{
    if (!adapter) {
        return ENPU_INVALID_PARAM;
    }
    if (adapter->destroy) {
        return adapter->destroy(adapter);
    }
    free(adapter);
    return ENPU_SUCCESS;
}