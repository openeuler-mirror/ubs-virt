/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 */

#include "swap_buffer_shm.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "log.h"

#define SWAP_BUFFER_MAX_RETRY_COUNT 50
#define SWAP_BUFFER_RETRY_INTERVAL_US 100000

/* 记录 attach 时获取的实际大小, 供 detach 时 munmap 使用 */
static uint64_t g_attached_size = 0;

void *swap_buffer_shm_attach(void)
{
    const char *shm_name = SHM_SWAP_BUFFER_NAME;

    void *base = NULL;
    int fd = -1;

    for (int retry = 0; retry < SWAP_BUFFER_MAX_RETRY_COUNT; retry++) {
        fd = shm_open(shm_name, O_RDWR, 0644);
        if (fd < 0) {
            if (retry < SWAP_BUFFER_MAX_RETRY_COUNT - 1) {
                LOG_DEBUG("swap_buffer shm %s not found, retry %d/%d", shm_name, retry + 1,
                          SWAP_BUFFER_MAX_RETRY_COUNT);
                usleep(SWAP_BUFFER_RETRY_INTERVAL_US);
                continue;
            }
            LOG_ERROR("swap_buffer shm %s not available after %d retries: %s", shm_name, SWAP_BUFFER_MAX_RETRY_COUNT,
                      strerror(errno));
            return NULL;
        }

        struct stat st;
        if (fstat(fd, &st) != 0) {
            LOG_ERROR("swap_buffer shm %s fstat failed: %s", shm_name, strerror(errno));
            close(fd);
            return NULL;
        }

        /* enpu-manager 配多少就映射多少, 不再硬编码 只检查 size > 0（文件已创建且 ftruncate 过）*/
        if (st.st_size <= 0) {
            close(fd);
            fd = -1;
            if (retry < SWAP_BUFFER_MAX_RETRY_COUNT - 1) {
                LOG_DEBUG("swap_buffer shm %s size=%ld not ready, retry %d/%d", shm_name, (long)st.st_size, retry + 1,
                          SWAP_BUFFER_MAX_RETRY_COUNT);
                usleep(SWAP_BUFFER_RETRY_INTERVAL_US);
                continue;
            }
            LOG_ERROR("swap_buffer shm %s size=%ld (not ready) after %d retries", shm_name, (long)st.st_size,
                      SWAP_BUFFER_MAX_RETRY_COUNT);
            return NULL;
        }

        g_attached_size = (uint64_t)st.st_size;
        base = mmap(NULL, g_attached_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        fd = -1;
        if (base == MAP_FAILED) {
            LOG_ERROR("swap_buffer shm %s mmap(size=%lu) failed: %s", shm_name, (unsigned long)g_attached_size,
                      strerror(errno));
            g_attached_size = 0;
            base = NULL;
            return NULL;
        }

        break;
    }

    if (base == NULL) {
        LOG_ERROR("swap_buffer_shm_attach failed: no mapping for shm=%s", shm_name);
        return NULL;
    }

    LOG_INFO("swap_buffer_shm_attach success: shm=%s, base=%p, size=%lu", shm_name, base,
             (unsigned long)g_attached_size);
    return base;
}

int swap_buffer_shm_detach(void *base)
{
    if (base == NULL) {
        LOG_WARN("swap_buffer_shm_detach: base is NULL, nothing to detach");
        g_attached_size = 0;
        return ENPU_SUCCESS;
    }

    if (g_attached_size == 0) {
        LOG_WARN("swap_buffer_shm_detach: g_attached_size=0, cannot munmap safely");
        return ENPU_FAIL;
    }

    if (munmap(base, g_attached_size) != 0) {
        LOG_ERROR("swap_buffer_shm_detach munmap failed: base=%p, size=%lu, error=%s", base,
                  (unsigned long)g_attached_size, strerror(errno));
        return ENPU_FAIL;
    }

    LOG_INFO("swap_buffer_shm_detach success: base=%p, size=%lu", base, (unsigned long)g_attached_size);
    g_attached_size = 0;
    return ENPU_SUCCESS;
}
