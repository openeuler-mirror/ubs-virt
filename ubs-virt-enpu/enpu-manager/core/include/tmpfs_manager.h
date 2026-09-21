/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 */

#ifndef __TMPFS_MANAGER_H__
#define __TMPFS_MANAGER_H__

#include <stdint.h>

#define TMPFS_DEFAULT_PATH "/dev/shm"
#define TMPFS_MARGIN_PERCENT 20

#if defined(__cplusplus)
extern "C" {
#endif

int tmpfs_check_size(const char *path, uint64_t *current_size, uint64_t *free_size);
int tmpfs_adjust_size(const char *path, uint64_t required_size);
int tmpfs_verify_sufficient(const char *path, uint64_t required_size);

#if defined(__cplusplus)
}
#endif

#endif