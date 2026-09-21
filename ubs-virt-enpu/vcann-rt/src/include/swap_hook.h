/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 */

#ifndef __SWAP_HOOK_H__
#define __SWAP_HOOK_H__

#include <stdbool.h>
#include <stdint.h>
#include "common.h"
#include "memory_tracker.h"
#include "npu_manager.h"
#include "shm_manager.h"
#include "swap_executor.h"

typedef struct swap_hook {
    int vnpu_id;
    swap_executor_t *executor;
    memory_tracker_t *tracker;
} swap_hook_t;

#if defined(__cplusplus)
extern "C" {
#endif

swap_hook_t *swap_hook_create(int vnpu_id, void *executor, void *tracker);
int swap_hook_destroy(swap_hook_t *hook);

int swap_hook_malloc_mem(swap_hook_t *hook, void **ptr, uint64_t size);
int swap_hook_malloc_physical_mem(swap_hook_t *hook, rtDrvMemHandle handle, uint64_t size, rtDrvMemProp_t *prop,
                                  uint64_t flags);
int swap_hook_check_and_swap_in(swap_hook_t *hook);
int swap_hook_free_mem(swap_hook_t *hook, void *ptr);
int swap_hook_free_physical_mem(swap_hook_t *hook, rtDrvMemHandle handle);
int swap_hook_map_mem(swap_hook_t *hook, void *devPtr, size_t size, size_t offset, rtDrvMemHandle *handle,
                      uint64_t flags);
int swap_hook_unmap_mem(swap_hook_t *hook, void *devPtr);

int swap_hook_global_init(int vnpu_id);
void swap_hook_global_destroy(void);
swap_hook_t *swap_hook_get_global(void);

#if defined(__cplusplus)
}
#endif

#endif