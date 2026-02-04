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

#ifndef RUNTIME_STUB_H
#define RUNTIME_STUB_H

#if defined(__cplusplus)
extern "C" {
#endif

#define MOCK_MEMCTL_LOCK_PATH "../build/memctl.lock"

void load_rt_libraries(void);
void stub_load_rt_libraries(void);

const char *lock_path();
const char *stub_lock_path();

void enpu_global_init(void);

int get_mem_used(size_t *used);
void *map_share_mem(const char *shmID, size_t size);

typedef struct file_lock {
    int fd;
    bool held;
} file_lock;

file_lock file_lock_create(const char *path, int operation);
void file_lock_destroy(file_lock *lock);

#if defined(__cplusplus)
}
#endif

#endif // RUNTIME_STUB_H