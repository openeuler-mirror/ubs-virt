/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 */

#ifndef __POD_WATCHDOG_H__
#define __POD_WATCHDOG_H__

#include <stdbool.h>
#include <stdint.h>
#include "common.h"

#define POD_ALIVE_TIMEOUT_NS (5ULL * 1000ULL * 1000ULL * 1000ULL) /* 5 秒 */

typedef struct pod_watchdog pod_watchdog_t;

#if defined(__cplusplus)
extern "C" {
#endif

pod_watchdog_t *pod_watchdog_create(void *shm_mgr, void *swap_buf_mgr, void *allocator);
int pod_watchdog_destroy(pod_watchdog_t *wd);

int pod_watchdog_cleanup_swap_data(pod_watchdog_t *wd, int phy_id, int vnpu_id);
int pod_watchdog_poll_and_cleanup(pod_watchdog_t *wd);

#if defined(__cplusplus)
}
#endif

#endif