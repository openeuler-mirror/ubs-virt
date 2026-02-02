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

#ifndef __MEMORY_LIMITER_H__
#define __MEMORY_LIMITER_H__

#include <inttypes.h>
#include <stddef.h>
#include "common.h"
#include "utils.h"
#include "npu_manager.h"


#define file_lock_BASE_DIR "/run/enpu/vcann-rt/"
#define MEMCTL_LOCK_PATH "/run/enpu/vcann-rt/memctl.lock"

typedef int32_t rtError_t;    // 兼容 runtime 接口
static const int32_t RT_ERROR_INVALID_VALUE = 0x07110001;

extern int guard_memory(size_t requested);
extern int memory_limiter_init();
extern bool memory_check(size_t requested);
extern const char *lock_path();
extern int create_file_lock_base_dir();

#endif