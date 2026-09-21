/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 */

#ifndef __SWAP_MONITOR_H__
#define __SWAP_MONITOR_H__

#include <stdbool.h>
#include <stdint.h>
#include "common.h"

#define SWAP_ACTION_NONE 0
#define SWAP_ACTION_OUT 1
#define SWAP_ACTION_IN 2

typedef struct swap_monitor swap_monitor_t;

#if defined(__cplusplus)
extern "C" {
#endif

swap_monitor_t *swap_monitor_create(int vnpu_id, void *executor, void *tracker);
int swap_monitor_destroy(swap_monitor_t *monitor);

int swap_monitor_poll(swap_monitor_t *monitor);

#if defined(__cplusplus)
}
#endif

#endif