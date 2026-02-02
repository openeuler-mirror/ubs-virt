/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * ubs-virt-ovs is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <acl/acl.h>
#include "mem_limiter.h"

bool memory_check(size_t requested)
{
    if (!is_mem_limit()) {
        return true;
    }
    size_t used;
    int ret = get_mem_used(&used);
    if (ret != 0) {
        LOG_ERROR("get mem used failed");
        return false;
    }
    size_t quota = get_mem_limit_quota();
    if (requested + used > quota) {
        LOG_ERROR("out of memory, request %zd B, used %zd B, quota %zd B", requested, used, quota);
        return false;
    }
    return true;
}

int guard_memory(size_t requested)
{
    // 创建文件锁

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

int create_file_lock_base_dir()
{
    int ret = mkdir(FILE_LOCK_BASE_DIR, S_IRWXU | S_IRGRP | S_IXGRP);
    if (ret < 0 && errno != EEXIST) {
        LOG_ERROR("mkdir %s failed, err is %d", FILE_LOCK_BASE_DIR, strerror(errno));
        return ENPU_FAIL;
    }

    LOG_INFO("mkdir %s success", FILE_LOCK_BASE_DIR);
    return ENPU_SUCCESS;
}


int memory_limiter_init()
{
    return create_file_lock_base_dir();
}