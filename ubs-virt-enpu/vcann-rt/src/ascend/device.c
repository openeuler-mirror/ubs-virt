/*
* Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
* ubs-virt-ovs is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v2 for more details.
*/

#include "log.h"
#include "runtime_hook.h"
#include "core_limiter.h"
#include "npu_manager.h"

pthread_once_t pre_rt_init_flag = PTHREAD_ONCE_INIT;

void load_rt_libraries(void)
{
    int i;
    for (i = 0; i < RUNTIME_ENTRY_END; i++) {
        rt_library_entry[i].func_ptr = dlsym(RTLD_NEXT, rt_library_entry[i].name);
        if (rt_library_entry[i].func_ptr == NULL) {
            LOG_ERROR("can't find function %s", rt_library_entry[i].name);
        }
    }
    return;
}

RUNTIME_HOOK_DEFINE(rtSetDevice, int32_t devId)
{
    enpu_global_init();
    LOG_INFO("Hook init rtSetDevice devId:%" PRIi32, devId);
    LOG_INFO("Hook modify cur VNPU_SCHEULE_PERIOD is: %zd, limit is %zd.",
        VNPU_SCHEULE_PERIOD / NS_PER_MS, get_core_limit_quota());
    pthread_once(&pre_rt_init_flag, load_rt_libraries);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtSetDevice, devId);
}

RUNTIME_HOOK_DEFINE(rtSetDeviceEx, int32_t devId)
{
    enpu_global_init();
    LOG_INFO("Hook init rtSetDeviceEx devId:%" PRIi32, devId);
    LOG_INFO("Hook modify cur VNPU_SCHEULE_PERIOD is: %zd, limit is %zd.",
        VNPU_SCHEULE_PERIOD / NS_PER_MS, get_core_limit_quota());
    pthread_once(&pre_rt_init_flag, load_rt_libraries);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtSetDeviceEx, devId);
}

RUNTIME_HOOK_DEFINE(rtSetDeviceWithFlags, int32_t devId, uint64_t flags)
{
    enpu_global_init();
    LOG_INFO("Hook init rtSetDeviceWithFlags devId:%" PRIi32, devId);
    LOG_INFO("Hook modify cur VNPU_SCHEULE_PERIOD is: %zd, limit is %zd.",
        VNPU_SCHEULE_PERIOD / NS_PER_MS, get_core_limit_quota());
    pthread_once(&pre_rt_init_flag, load_rt_libraries);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtSetDeviceWithFlags, devId, flags);
}
