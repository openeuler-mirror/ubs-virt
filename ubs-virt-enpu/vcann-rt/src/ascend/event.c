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

#include "log.h"
#include "runtime_hook.h"
#include "core_limiter.h"
#include "npu_manager.h"

RUNTIME_HOOK_DEFINE(rtEventCreate, rtEvent_t *evt)
{
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtEventCreate, evt);
    if (ret == ACL_RT_SUCCESS && is_core_limit()) {
        set_event_create_status(*evt);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtsEventCreate, rtEvent_t *evt, uint64_t flag)
{
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsEventCreate, evt, flag);
    if (ret == ACL_RT_SUCCESS && is_core_limit()) {
        set_event_create_status(*evt);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtsEventCreateEx, rtEvent_t *evt, uint64_t flag)
{
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsEventCreateEx, evt, flag);
    if (ret == ACL_RT_SUCCESS && is_core_limit()) {
        set_event_create_status(*evt);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtEventCreateWithFlag, rtEvent_t *evt, uint32_t flag)
{
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtEventCreateWithFlag, evt, flag);
    if (ret == ACL_RT_SUCCESS && is_core_limit()) {
        set_event_create_status(*evt);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtEventCreateExWithFlag, rtEvent_t *evt, uint32_t flag)
{
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtEventCreateExWithFlag, evt, flag);
    if (ret == ACL_RT_SUCCESS && is_core_limit()) {
        set_event_create_status(*evt);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtStreamWaitEvent, rtStream_t stm, rtEvent_t evt)
{
    core_limiter(stm, set_event_wait_status, (void*)evt);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtStreamWaitEvent, stm, evt);
    return ret;
}

RUNTIME_HOOK_DEFINE(rtEventRecord, rtEvent_t evt, rtStream_t stm)
{
    core_limiter(stm, NULL, NULL);
    if (is_core_limit()) {
        set_event_record_status(evt, stm);
    }
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtEventRecord, evt, stm);
    return ret;
}

RUNTIME_HOOK_DEFINE(rtEventDestroy, rtEvent_t evt)
{
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtEventDestroy, evt);
    if (ret == ACL_RT_SUCCESS && is_core_limit()) {
        set_event_destroy_status(evt);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtStreamDestroy, rtStream_t stm)
{
    core_limiter(stm, remove_stream, NULL);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtStreamDestroy, stm);
    return ret;
}

RUNTIME_HOOK_DEFINE(rtDestroyStreamForce, rtStream_t stm)
{
    core_limiter(stm, remove_stream, NULL);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtDestroyStreamForce, stm);
    return ret;
}

RUNTIME_HOOK_DEFINE(rtsNotifyCreate, rtNotify_t *notify, uint64_t flag)
{
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsNotifyCreate, notify, flag);
    if (ret == ACL_RT_SUCCESS && is_core_limit()) {
        set_event_create_status(*notify);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtNotifyRecord, rtNotify_t evt, rtStream_t stm)
{
    core_limiter(stm, NULL, NULL);
    if (is_core_limit()) {
        set_event_record_status(evt, stm);
    }
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtNotifyRecord, evt, stm);
    return ret;
}

RUNTIME_HOOK_DEFINE(rtNotifyDestroy, rtNotify_t evt)
{
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtNotifyDestroy, evt);
    if (ret == ACL_RT_SUCCESS && is_core_limit()) {
        set_event_destroy_status(evt);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtsNotifyWaitAndReset, rtNotify_t notify, rtStream_t stm, uint32_t timeout)
{
    core_limiter(stm, set_event_wait_status, (void*)notify);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsNotifyWaitAndReset, notify, stm, timeout);
    return ret;
}

RUNTIME_HOOK_DEFINE(rtStreamWaitEventWithTimeout, rtStream_t stm, rtEvent_t evt, uint32_t timeout)
{
    core_limiter(stm, set_event_wait_status, (void*)evt);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtStreamWaitEventWithTimeout, stm, evt, timeout);
}

RUNTIME_HOOK_DEFINE(rtEventDestroySync, rtEvent_t evt)
{
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtEventDestroySync, evt);
    if (ret == ACL_RT_SUCCESS && is_core_limit()) {
        set_event_destroy_status(evt);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtNotifyCreate, int32_t deviceId, rtNotify_t *notify)
{
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtNotifyCreate, deviceId, notify);
    if (ret == ACL_RT_SUCCESS && is_core_limit()) {
        set_event_create_status(*notify);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtNotifyCreateWithFlag, int32_t deviceId, rtNotify_t *notify, uint32_t flag)
{
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtNotifyCreateWithFlag, deviceId, notify, flag);
    if (ret == ACL_RT_SUCCESS && is_core_limit()) {
        set_event_create_status(*notify);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtNotifyWait, rtNotify_t notify, rtStream_t stm)
{
    core_limiter(stm, set_event_wait_status, (void*)notify);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtNotifyWait, notify, stm);
}

RUNTIME_HOOK_DEFINE(rtNotifyWaitWithTimeOut, rtNotify_t notify, rtStream_t stm, uint32_t timeOut)
{
    core_limiter(stm, set_event_wait_status, (void*)notify);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtNotifyWaitWithTimeOut, notify, stm, timeOut);
}
