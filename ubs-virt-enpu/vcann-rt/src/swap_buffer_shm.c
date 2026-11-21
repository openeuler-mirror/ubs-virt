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

int swap_buffer_shm_detach(swap_buffer_mapping_t *mapping)
{
    if (mapping->base == NULL) {
        return ENPU_SUCCESS;
    }

    if (munmap(mapping->base, mapping->size) != 0) {
        LOG_ERROR("munmap failed: base=%p, size=%zu, error=%s", mapping->base, mapping->size, strerror(errno));
        return ENPU_FAIL;
    }

    LOG_INFO("swap_buffer_shm_detach success: base=%p, size=%zu", mapping->base, mapping->size);
    *mapping = (swap_buffer_mapping_t){0};
    return ENPU_SUCCESS;
}

static int swap_buffer_shm_attach_internal(const char *shm_name, int max_attempts, swap_buffer_mapping_t *mapping)
{
    if (shm_name == NULL || shm_name[0] == '\0' || mapping == NULL || mapping->base != NULL) {
        LOG_ERROR("swap_buffer_shm_attach_by_name: invalid arguments");
        return ENPU_FAIL;
    }

    for (int retry = 0; retry < max_attempts; retry++) {
        int fd = shm_open(shm_name, O_RDWR, 0644);
        if (fd < 0) {
            if (retry < max_attempts - 1) {
                LOG_DEBUG("swap_buffer shm %s not found, retry %d/%d", shm_name, retry + 1, max_attempts);
                usleep(SWAP_BUFFER_RETRY_INTERVAL_US);
                continue;
            }
            LOG_ERROR("swap_buffer shm %s not available after %d retries: %s", shm_name, max_attempts, strerror(errno));
            return ENPU_FAIL;
        }

        struct stat st;
        if (fstat(fd, &st) != 0) {
            LOG_ERROR("swap_buffer shm %s fstat failed: %s", shm_name, strerror(errno));
            close(fd);
            return ENPU_FAIL;
        }

        /* enpu-manager 配多少就映射多少，不再硬编码 只检查 size > 0（文件已创建且 ftruncate 过）*/
        if (st.st_size <= 0) {
            close(fd);
            if (retry < max_attempts - 1) {
                LOG_DEBUG("swap_buffer shm %s size=%ld not ready, retry %d/%d", shm_name, (long)st.st_size, retry + 1,
                          max_attempts);
                usleep(SWAP_BUFFER_RETRY_INTERVAL_US);
                continue;
            }
            LOG_ERROR("swap_buffer shm %s size=%ld (not ready) after %d retries", shm_name, (long)st.st_size,
                      max_attempts);
            return ENPU_FAIL;
        }

        size_t size = (size_t)st.st_size;
        void *base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        if (base == MAP_FAILED) {
            LOG_ERROR("swap_buffer shm %s mmap(size=%zu) failed: %s", shm_name, size, strerror(errno));
            return ENPU_FAIL;
        }

        *mapping = (swap_buffer_mapping_t){.base = base, .size = size};
        LOG_INFO("swap_buffer_shm_attach_by_name success: shm=%s, base=%p, size=%zu", shm_name, base, size);
        return ENPU_SUCCESS;
    }
    return ENPU_FAIL;
}

int swap_buffer_shm_attach_by_name(const char *shm_name, swap_buffer_mapping_t *mapping)
{
    return swap_buffer_shm_attach_internal(shm_name, SWAP_BUFFER_MAX_RETRY_COUNT, mapping);
}

int swap_buffer_shm_try_attach_by_name(const char *shm_name, swap_buffer_mapping_t *mapping)
{
    return swap_buffer_shm_attach_internal(shm_name, 1, mapping);
}
