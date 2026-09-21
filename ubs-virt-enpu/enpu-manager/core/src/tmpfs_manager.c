/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 */

#include "tmpfs_manager.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include "common.h"
#include "log.h"
#include "securec.h"

int tmpfs_check_size(const char *path, uint64_t *current_size, uint64_t *free_size)
{
    if (path == NULL) {
        path = TMPFS_DEFAULT_PATH;
    }

    struct statvfs st;
    if (statvfs(path, &st) != 0) {
        LOG_ERROR("statvfs(%s) failed: %s", path, strerror(errno));
        return ENPU_FAIL;
    }

    if (current_size != NULL) {
        *current_size = (uint64_t)st.f_blocks * st.f_frsize;
    }

    if (free_size != NULL) {
        *free_size = (uint64_t)st.f_bavail * st.f_frsize;
    }

    return ENPU_SUCCESS;
}

int tmpfs_adjust_size(const char *path, uint64_t required_size)
{
    if (path == NULL) {
        path = TMPFS_DEFAULT_PATH;
    }

    uint64_t current_size = 0;
    int ret = tmpfs_check_size(path, &current_size, NULL);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }

    uint64_t target_size = required_size * (100 + TMPFS_MARGIN_PERCENT) / 100;

    if (current_size >= target_size) {
        LOG_INFO("tmpfs size sufficient: current=%lu, required=%lu (with %d%% margin)", current_size, target_size,
                 TMPFS_MARGIN_PERCENT);
        return ENPU_SUCCESS;
    }

    /* 内核 memparse 不识别小数，带小数的 "%.2fG"（如 "size=2.10G"）会让 remount 以 EINVAL 失败；
     * 这里直接按字节数下发，memparse 对纯数字按字节解析 */
    char size_str[64];
    int fmt_ret = snprintf_s(size_str, sizeof(size_str), sizeof(size_str) - 1, "size=%lu", target_size);
    if (fmt_ret < 0) {
        LOG_ERROR("format tmpfs size string failed");
        return ENPU_FAIL;
    }

    LOG_INFO("adjusting tmpfs size: current=%lu, target=%lu (%s)", current_size, target_size, size_str);

    if (mount(NULL, path, NULL, MS_REMOUNT, size_str) != 0) {
        LOG_ERROR("mount remount failed: path='%s', opts='%s', errno=%d (%s)", path, size_str, errno, strerror(errno));
        return ENPU_FAIL;
    }

    uint64_t new_size = 0;
    ret = tmpfs_check_size(path, &new_size, NULL);
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("failed to verify new tmpfs size after adjustment");
        return ENPU_FAIL;
    }

    if (new_size < target_size) {
        LOG_ERROR("tmpfs adjustment did not reach target: new=%lu, target=%lu", new_size, target_size);
        return ENPU_FAIL;
    }

    LOG_INFO("tmpfs adjusted successfully: new_size=%lu", new_size);
    return ENPU_SUCCESS;
}

int tmpfs_verify_sufficient(const char *path, uint64_t required_size)
{
    if (path == NULL) {
        path = TMPFS_DEFAULT_PATH;
    }

    if (required_size == 0) {
        LOG_INFO("required_size=0, skip tmpfs verification");
        return ENPU_SUCCESS;
    }

    uint64_t current_size = 0;
    uint64_t free_size = 0;
    int ret = tmpfs_check_size(path, &current_size, &free_size);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }

    uint64_t target_size = required_size * (100 + TMPFS_MARGIN_PERCENT) / 100;

    if (current_size >= target_size) {
        LOG_INFO("tmpfs sufficient: current=%lu, target=%lu (required=%lu + %d%% margin)", current_size, target_size,
                 required_size, TMPFS_MARGIN_PERCENT);
        return ENPU_SUCCESS;
    }

    LOG_INFO("tmpfs insufficient: current=%lu, target=%lu, attempting adjustment", current_size, target_size);

    return tmpfs_adjust_size(path, required_size);
}