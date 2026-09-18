/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 */

#ifndef __SWAP_RESOLVER_THREAD_H__
#define __SWAP_RESOLVER_THREAD_H__

#include <stdbool.h>
#include <stdint.h>
#include "common.h"

typedef struct swap_resolver_thread swap_resolver_thread_t;

#if defined(__cplusplus)
extern "C" {
#endif

swap_resolver_thread_t *swap_resolver_thread_create(void *resolver, int phy_id, uint64_t poll_interval_ms);
int swap_resolver_thread_destroy(swap_resolver_thread_t *thread);

int swap_resolver_thread_start(swap_resolver_thread_t *thread);
int swap_resolver_thread_stop(swap_resolver_thread_t *thread);

#if defined(__cplusplus)
}
#endif

#endif