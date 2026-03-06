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

#define _GNU_SOURCE
#include <dlfcn.h>
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
            LOG_INFO("Failed to find function %s, because the runtime version you are using is different "
            "from our preset version.", rt_library_entry[i].name);
        }
    }
    return;
}

RUNTIME_HOOK_DEFINE(rtSetDevice, int32_t devId)
{
    enpu_global_init();
    if (devId != 0) {
        LOG_WARN("SetDevice should only pass devId=0. And devId will be overwrited to %d.", get_device_id());
    }

    devId = get_device_id();
    LOG_DEBUG("Hook init rtSetDevice devId:%" PRIi32 ".", devId);
    LOG_DEBUG("The total time slice length is: %zd, and %zd %% of it is available.",
        VNPU_SCHEULE_PERIOD / NS_PER_MS, get_core_limit_quota());
    pthread_once(&pre_rt_init_flag, load_rt_libraries);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtSetDevice, devId);
    CHECK_COND_RETURN_((ret != ACL_RT_SUCCESS), ret, "Call rtSetDevice fails, ret:%d.", ret);
    enpu_global_init_post();
    return ACL_RT_SUCCESS;
}

RUNTIME_HOOK_DEFINE(rtSetDeviceEx, int32_t devId)
{
    enpu_global_init();
    if (devId != 0) {
        LOG_WARN("SetDevice should only pass devId=0. And devId will be overwrited to %d.", get_device_id());
    }

    devId = get_device_id();
    LOG_DEBUG("Hook init rtSetDeviceEx devId:%" PRIi32 ".", devId);
    LOG_DEBUG("The total time slice length is: %zd, and %zd %% of it is available.",
        VNPU_SCHEULE_PERIOD / NS_PER_MS, get_core_limit_quota());
    pthread_once(&pre_rt_init_flag, load_rt_libraries);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtSetDeviceEx, devId);
    CHECK_COND_RETURN_((ret != ACL_RT_SUCCESS), ret, "Call rtSetDeviceEx fails, ret:%d.", ret);
    enpu_global_init_post();
    return ACL_RT_SUCCESS;
}

RUNTIME_HOOK_DEFINE(rtSetDeviceWithFlags, int32_t devId, uint64_t flags)
{
    enpu_global_init();
    if (devId != 0) {
        LOG_WARN("SetDevice should only pass devId=0. And devId will be overwrited to %d.", get_device_id());
    }

    devId = get_device_id();
    LOG_DEBUG("Hook init rtSetDeviceWithFlags devId:%" PRIi32 ".", devId);
    LOG_DEBUG("The total time slice length is: %zd, and %zd %% of it is available.",
        VNPU_SCHEULE_PERIOD / NS_PER_MS, get_core_limit_quota());
    pthread_once(&pre_rt_init_flag, load_rt_libraries);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtSetDeviceWithFlags, devId, flags);
    CHECK_COND_RETURN_((ret != ACL_RT_SUCCESS), ret, "Call rtSetDeviceWithFlags fails, ret:%d.", ret);
    enpu_global_init_post();
    return ACL_RT_SUCCESS;
}

RUNTIME_HOOK_DEFINE(rtSetDeviceWithoutTsd, int32_t devId)
{
    enpu_global_init();
    if (devId != 0) {
        LOG_WARN("SetDevice should only pass devId=0. And devId will be overwrited to %d.", get_device_id());
    }

    devId = get_device_id();
    LOG_DEBUG("Hook init rtSetDeviceWithoutTsd devId:%" PRIi32 ".", devId);
    LOG_DEBUG("The total time slice length is: %zd, and %zd %% of it is available.",
        VNPU_SCHEULE_PERIOD / NS_PER_MS, get_core_limit_quota());
    pthread_once(&pre_rt_init_flag, load_rt_libraries);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtSetDeviceWithoutTsd, devId);
    CHECK_COND_RETURN_((ret != ACL_RT_SUCCESS), ret, "Call rtSetDeviceWithoutTsd fails, ret:%d.", ret);
    enpu_global_init_post();
    return ACL_RT_SUCCESS;
}
