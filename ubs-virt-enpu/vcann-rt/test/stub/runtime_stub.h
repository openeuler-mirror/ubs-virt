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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define MOCK_MEMCTL_LOCK_PATH "../__build/memctl.lock"
#define MOCK_NPU_CONFIG_PATH "../__build/test_npu_info.config"

// device.c
void load_rt_libraries(void);
void stub_load_rt_libraries(void);

// mem_limiter.c
const char *stub_lock_path();

// npu_manager.c
int stub_load_config(const char *file_path);
int stub_enpu_load_config(void);

// core_limiter.c
void *npu_utilization_monitor_thread(void *arg);
bool slide_window_check(int owner);
void check_and_borrow_timeslice(int owner);
int calculate_alive_vnpu_num(void);
bool stub_is_core_limiter(void);

// config.c
int check_int32(int32_t option, const char *option_name);
int check_str(const char *str, const char *option_name);
int load_str(const char *key, const char *value, char *ret_value, size_t ret_len);

int stub_get_random_fd(void);
ssize_t stub_read_fail(int fd, void *buf, size_t size);
ssize_t stub_read_success(int fd, void *buf, size_t size);
bool stub_is_random_sampling_true(void);
bool stub_is_random_sampling_false(void);

#if defined(__cplusplus)
}
#endif

#endif // RUNTIME_STUB_H