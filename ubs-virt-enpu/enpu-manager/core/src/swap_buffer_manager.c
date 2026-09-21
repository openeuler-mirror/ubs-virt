/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 */

#include "swap_buffer_manager.h"
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "log.h"
#include "shm_manager.h"

int swap_buffer_manager_get_status(swap_buffer_manager_t *mgr, swap_buffer_status_t *status)
{
    if (mgr == NULL || status == NULL) {
        return ENPU_INVALID_PARAM;
    }

    return shm_swap_buffer_get_status(status);
}

#define SWAP_BUFFER_SHM_FD_INVALID (-1)
static int g_swap_buffer_shm_fd = SWAP_BUFFER_SHM_FD_INVALID;
static uint64_t g_swap_buffer_shm_size = 0;

int swap_buffer_shm_open(void)
{
    return shm_open(SWAP_BUFFER_SHM_NAME, O_CREAT | O_RDWR, 0644);
}

int swap_buffer_shm_truncate(int fd, uint64_t size)
{
    return ftruncate(fd, (off_t)size);
}

int swap_buffer_shm_unlink(void)
{
    return shm_unlink(SWAP_BUFFER_SHM_NAME);
}

int swap_buffer_posix_shm_create(uint64_t size)
{
    if (size == 0) {
        return ENPU_INVALID_PARAM;
    }
    if (g_swap_buffer_shm_fd != SWAP_BUFFER_SHM_FD_INVALID) {
        return ENPU_ALREADY_EXISTS;
    }

    int fd = swap_buffer_shm_open();
    if (fd < 0) {
        return ENPU_FAIL;
    }
    if (swap_buffer_shm_truncate(fd, size) < 0) {
        close(fd);
        return ENPU_FAIL;
    }
    g_swap_buffer_shm_fd = fd;
    g_swap_buffer_shm_size = size;
    return ENPU_SUCCESS;
}

int swap_buffer_posix_shm_destroy(void)
{
    if (g_swap_buffer_shm_fd != SWAP_BUFFER_SHM_FD_INVALID) {
        close(g_swap_buffer_shm_fd);
        g_swap_buffer_shm_fd = SWAP_BUFFER_SHM_FD_INVALID;
    }
    swap_buffer_shm_unlink();
    g_swap_buffer_shm_size = 0;
    return ENPU_SUCCESS;
}

int swap_buffer_posix_shm_get_size(uint64_t *size)
{
    if (size == NULL) {
        return ENPU_INVALID_PARAM;
    }
    *size = g_swap_buffer_shm_size;
    return ENPU_SUCCESS;
}
