/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 */

#ifndef __SWAP_BUFFER_MANAGER_H__
#define __SWAP_BUFFER_MANAGER_H__

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include "common.h"

typedef struct swap_buffer_status {
    uint64_t used;
    uint64_t free;
    uint64_t total;
} swap_buffer_status_t;

typedef struct swap_buffer_manager {
    void *swap_buffer_base;
    uint64_t buffer_size;
    pthread_mutex_t lock;
} swap_buffer_manager_t;

#if defined(__cplusplus)
extern "C" {
#endif

int swap_buffer_manager_get_status(swap_buffer_manager_t *mgr, swap_buffer_status_t *status);

#define SWAP_BUFFER_SHM_NAME "/swap_buffer_total"

int swap_buffer_posix_shm_create(uint64_t size);
int swap_buffer_posix_shm_destroy(void);
int swap_buffer_posix_shm_get_size(uint64_t *size);

/* shm 的名字、打开 flags、权限位只在本模块定义一份，TLSF 池等模块统一走下面三个原语，
 * 避免各处各自 shm_open 导致约定漂移。
 * open 与 truncate 不合并成单个函数：两处调用方失败后的清理动作不同
 * （posix_shm_create 只 close，tlsf_create_with_shm 还要 unlink），合并等于把一方的清理强加给另一方。
 * swap_buffer_shm_open 成功返回 fd，失败返回 -1 */
int swap_buffer_shm_open(void);
int swap_buffer_shm_truncate(int fd, uint64_t size);
int swap_buffer_shm_unlink(void);

#if defined(__cplusplus)
}
#endif

#endif
