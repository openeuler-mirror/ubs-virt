/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include "mem_limiter.h"
#include <acl/acl.h>
#include <errno.h>
#include <runtime/rt.h>
#include <string.h>
#include <sys/stat.h>
#include "runtime_hook.h"

bool memory_check(size_t requested)
{
    size_t used = 0;
    int ret = get_mem_used(&used);
    CHECK_COND_RETURN_(ret != 0, false, "get mem used failed.");
    size_t quota = get_mem_limit_quota();
    size_t total;
    bool result = __builtin_add_overflow(requested, used, &total);
    CHECK_COND_RETURN_(result, false, "User requested mem size too big! Request:%zu B, used:%zu B, quota:%zu B.",
                       requested, used, quota);
    CHECK_COND_RETURN_((total > quota), false, "Out of memory! Request:%zu B, used:%zu B, quota:%zu B.", requested,
                       used, quota);
    return true;
}

int guard_memory(size_t requested)
{
    file_lock lock = file_lock_create(lock_path(), LOCK_EX);
    if (!file_lock_isvalid(&lock)) {
        LOG_ERROR("Guard memory failed to create file lock!");
        file_lock_destroy(&lock);
        return ACL_ERROR_FAILURE;
    }

    if (!memory_check(requested)) {
        LOG_ERROR("Guard memory out of memory error for requested:%zd", requested);
        file_lock_destroy(&lock);
        return ACL_ERROR_STORAGE_OVER_LIMIT;
    }
    file_lock_destroy(&lock);
    return ENPU_SUCCESS;
}

const char *lock_path()
{
    return MEMCTL_LOCK_PATH;
}

/* 锁目录权限 0750：owner rwx / group r-x / other ---。group 不可写，否则同组进程
 * 能 unlink 掉 memctl.lock 另建一个，破坏 guard_memory 的跨进程互斥。 */
#define FILE_LOCK_DIR_MODE (S_IRWXU | S_IRGRP | S_IXGRP)

static int mkdir_recursive(const char *path, mode_t mode)
{
    char tmp[FILE_PATH_LEN];
    size_t len = strlen(path);
    if (len == 0 || len >= sizeof(tmp)) {
        return ENPU_FAIL;
    }
    if (strcpy_s(tmp, sizeof(tmp), path) != 0) {
        return ENPU_FAIL;
    }
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }

    for (char *p = tmp + 1; *p != '\0'; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, mode) != 0 && errno != EEXIST) {
                return ENPU_FAIL;
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, mode) != 0 && errno != EEXIST) {
        return ENPU_FAIL;
    }
    return ENPU_SUCCESS;
}

int create_file_lock_base_dir()
{
    const char *path = lock_path();
    if (path == NULL) {
        LOG_ERROR("lock_path() returned NULL.");
        return ENPU_FAIL;
    }

    char dir[FILE_PATH_LEN];
    if (strcpy_s(dir, sizeof(dir), path) != 0) {
        LOG_ERROR("Failed to copy lock path.");
        return ENPU_FAIL;
    }

    char *last_slash = strrchr(dir, '/');
    if (last_slash == NULL) {
        LOG_ERROR("Invalid lock path (no directory component): %s.", path);
        return ENPU_FAIL;
    }
    *last_slash = '\0';

    if (strlen(dir) == 0) {
        LOG_ERROR("Empty directory path derived from %s.", path);
        return ENPU_FAIL;
    }

    int ret = mkdir_recursive(dir, FILE_LOCK_DIR_MODE);
    if (ret != ENPU_SUCCESS) {
        char errbuf[128] = {0};
        (void)strerror_r(errno, errbuf, sizeof(errbuf));
        LOG_ERROR("create %s failed, err is %s.", dir, errbuf);
        return ENPU_FAIL;
    }

    LOG_INFO("create %s success", dir);
    return ENPU_SUCCESS;
}

int memory_limiter_init()
{
    return create_file_lock_base_dir();
}