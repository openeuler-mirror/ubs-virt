/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 */

#ifndef __SWAP_EXECUTOR_H__
#define __SWAP_EXECUTOR_H__

#include <stdbool.h>
#include <stdint.h>
#include "mem_limiter.h"
#include "memory_tracker.h"
#include "runtime_hook.h"

#define RT_ERROR_MEMORY_ALLOC_FAILED 2
#define APP_MODE_ID_U16 33
#define MEMCPY_FLAG 0
#define NOT_MEMCPY_FLAG 1

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct swap_executor swap_executor_t;

swap_executor_t *swap_executor_create(void *swap_buffer_base);
int swap_executor_destroy(swap_executor_t *executor);

int swap_executor_swap_out(swap_executor_t *executor, memory_tracker_t *tracker, memory_record_t **records, int count,
                           uint64_t swap_offset, uint64_t *act_swapped);

int swap_executor_swap_in(swap_executor_t *executor, void **ptr, uint64_t offset, uint64_t *size,
                          rtDrvMemHandle *new_handle, uint8_t flag);

int swap_executor_swap_in_physical(swap_executor_t *executor, memory_tracker_t *tracker, rtDrvMemHandle handle,
                                   uint64_t offset, uint64_t *size, rtDrvMemHandle *new_handle);

#if defined(__cplusplus)
}
#endif

#endif