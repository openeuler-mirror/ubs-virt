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

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <runtime/rt.h>
#include "runtime_hook.h"
#include "runtime_stub.h"

#define RUNTIME_STUB_FUNCTION_NAME(name) RUNTIME_STUB_##name

#define RUNTIME_FUNCTION_ENTRY(name, ...)                   \
    rtError_t RUNTIME_STUB_FUNCTION_NAME(name)(__VA_ARGS__) \
    {                                                       \
        printf("call stub function " #name "\n");           \
        return RT_ERROR_NONE;                               \
    }
RUNTIME_FUNCTION_LIST
#undef RUNTIME_FUNCTION_ENTRY

void stub_load_rt_libraries(void)
{
    printf("load all stub func\n");

#define RUNTIME_FUNCTION_ENTRY(name, ...) \
    rt_library_entry[RUNTIME_HOOK_ENUM(name)].func_ptr = (void *)RUNTIME_STUB_FUNCTION_NAME(name);
    RUNTIME_FUNCTION_LIST
#undef RUNTIME_FUNCTION_ENTRY
}

const char *stub_lock_path()
{
    printf("enter stub_lock_path");
    return MOCK_MEMCTL_LOCK_PATH;
}

int fail_stub_get_mem_used(size_t *used)
{
    (void)used;
    return -1;
}
