/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
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

RUNTIME_HOOK_DEFINE(rtEventCreate, rtEvent_t *evt)
{
    uint64_t beginTime = ns_now();
    LOG_DEBUG("Calling rtEventCreate at time %" PRIu64 "", beginTime);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtEventCreate, evt);
    uint64_t endTime = ns_now();
    LOG_DEBUG("Create rtEventCreate %p at time %" PRIu64 "", (void*)(*evt), endTime);
    if (ret == ACL_RT_SUCCESS) {
        set_event_create_status(*evt);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtsEventCreate, rtEvent_t *evt, uint64_t flag)
{
    uint64_t beginTime = ns_now();
    LOG_DEBUG("Calling rtsEventCreate at time %" PRIu64 "", beginTime);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsEventCreate, evt, flag);
    uint64_t endTime = ns_now();
    LOG_DEBUG("Create rtsEventCreate %p at time %" PRIu64 "", (void*)(*evt), endTime);
    if (ret == ACL_RT_SUCCESS) {
        set_event_create_status(*evt);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtsEventCreateEx, rtEvent_t *evt, uint64_t flag)
{
    uint64_t beginTime = ns_now();
    LOG_DEBUG("Calling rtsEventCreateEx at time %" PRIu64 "", beginTime);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsEventCreateEx, evt, flag);
    uint64_t endTime = ns_now();
    LOG_DEBUG("Create rtsEventCreateEx %p at time %" PRIu64 "", (void*)(*evt), endTime);
    if (ret == ACL_RT_SUCCESS) {
        set_event_create_status(*evt);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtEventCreateWithFlag, rtEvent_t *evt, uint32_t flag)
{
    uint64_t beginTime = ns_now();
    LOG_DEBUG("Calling rtEventCreateWithFlag at time %" PRIu64 "", beginTime);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtEventCreateWithFlag, evt, flag);
    uint64_t endTime = ns_now();
    LOG_DEBUG("Create rtEventCreateWithFlag %p at time %" PRIu64 "", (void*)(*evt), endTime);
    if (ret == ACL_RT_SUCCESS) {
        set_event_create_status(*evt);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtEventCreateExWithFlag, rtEvent_t *evt, uint32_t flag)
{
    uint64_t beginTime = ns_now();
    LOG_DEBUG("Calling rtEventCreateExWithFlag at time %" PRIu64, beginTime);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtEventCreateExWithFlag, evt, flag);
    uint64_t endTime = ns_now();
    LOG_DEBUG("Create rtEventCreateExWithFlag %p at time %" PRIu64 "", (void*)(*evt), endTime);
    if (ret == ACL_RT_SUCCESS) {
        set_event_create_status(*evt);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtStreamWaitEvent, rtStream_t stm, rtEvent_t evt)
{
    core_limiter(stm, set_event_wait_status, (void*)evt);
    uint64_t beginTime = ns_now();
    LOG_DEBUG("Stream %p calling rtStreamWaitEvent %p at time %" PRIu64 "", (void*)stm, (void*)evt, beginTime);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtStreamWaitEvent, stm, evt);
    uint64_t endTime = ns_now();
    return ret;
}

RUNTIME_HOOK_DEFINE(rtEventRecord, rtEvent_t evt, rtStream_t stm)
{
    core_limiter(stm, NULL, NULL);
    set_event_record_status(evt, stm);
    uint64_t beginTime = ns_now();
    LOG_DEBUG("Stream %p calling rtEventRecord %p at time %" PRIu64 "", (void*)stm, (void*)evt, beginTime);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtEventRecord, evt, stm);
    uint64_t endTime = ns_now();
    return ret;
}

RUNTIME_HOOK_DEFINE(rtEventDestroy, rtEvent_t evt)
{
    uint64_t beginTime = ns_now();
    LOG_DEBUG("Calling rtEventDestroy %p at time %" PRIu64 "", (void*)evt, beginTime);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtEventDestroy, evt);
    uint64_t endTime = ns_now();
    if (ret == ACL_RT_SUCCESS) {
        LOG_DEBUG("Ret is success.");
        set_event_destroy_status(evt);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtStreamDestroy, rtStream_t stm)
{
    core_limiter(stm, remove_stream, NULL);
    LOG_DEBUG("Calling rtStreamDestroy %p at time %" PRIu64 "", (void*)stm, ns_now());
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtStreamDestroy, stm);
    return ret;
}

RUNTIME_HOOK_DEFINE(rtDestroyStreamForce, rtStream_t stm)
{
    core_limiter(stm, remove_stream, NULL);
    LOG_DEBUG("Calling rtDestroyStreamForce %p at time %" PRIu64 "", (void*)stm, ns_now());
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtDestroyStreamForce, stm);
    return ret;
}

RUNTIME_HOOK_DEFINE(rtsNotifyCreate, rtNotify_t *notify, uint64_t flag)
{
    uint64_t beginTime = ns_now();
    LOG_DEBUG("Calling rtsNotifyCreate at time %" PRIu64 "", beginTime);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsNotifyCreate, notify, flag);
    uint64_t endTime = ns_now();
    LOG_DEBUG("Create rtsNotifyCreate %p at time %" PRIu64 "", (void*)(*notify), endTime);
    if (ret == ACL_RT_SUCCESS) {
        set_event_create_status(*notify);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtNotifyRecord, rtNotify_t evt, rtStream_t stm)
{
    core_limiter(stm, NULL, NULL);
    set_event_record_status(evt, stm);
    uint64_t beginTime = ns_now();
    LOG_DEBUG("Stream %p calling rtNotifyRecord %p at time %" PRIu64 "", (void*)stm, (void*)evt, beginTime);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtNotifyRecord, evt, stm);
    uint64_t endTime = ns_now();
    return ret;
}

RUNTIME_HOOK_DEFINE(rtNotifyDestroy, rtNotify_t evt)
{
    uint64_t beginTime = ns_now();
    LOG_DEBUG("Calling rtNotifyDestroy %p at time %" PRIu64 "", (void*)evt, beginTime);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtNotifyDestroy, evt);
    uint64_t endTime = ns_now();
    if (ret == ACL_RT_SUCCESS) {
        LOG_DEBUG("ret is success.");
        set_event_destroy_status(evt);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtsNotifyWaitAndReset, rtNotify_t notify, rtStream_t stm, uint32_t timeout)
{
    core_limiter(stm, set_event_wait_status, (void*)notify);
    uint64_t beginTime = ns_now();
    LOG_DEBUG("Stream %p calling rtsNotifyWaitAndReset %p at time %" PRIu64 "", (void*)stm, (void*)notify, beginTime);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsNotifyWaitAndReset, notify, stm, timeout);
    uint64_t endTime = ns_now();
    return ret;
}