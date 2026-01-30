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

RUNTIME_HOOK_DEFINE(rtModelExecute, rtModel_t mdl, rtStream_t stm, uint32_t flag)
{
    core_limiter(stm, NULL, NULL);
    uint64_t beginTime = ns_now();
    LOG_DEBUG("Calling rtModelExecute at time %" PRIu64 "", beginTime);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtModelExecute, mdl, stm, flag);
    uint64_t endTime = ns_now();
    LOG_DEBUG("Calling rtModelExecute use time: %" PRIu64 " ns", endTime - beginTime);
    return ret;
}

RUNTIME_HOOK_DEFINE(rtModelExecuteAsync, rtModel_t mdl, rtStream_t stm, uint32_t flag)
{
    core_limiter(stm, NULL, NULL);
    uint64_t beginTime = ns_now();
    LOG_DEBUG("Calling rtModelExecuteAsync at time %" PRIu64 "", beginTime);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtModelExecuteAsync, mdl, stm, flag);
    uint64_t endTime = ns_now();
    LOG_DEBUG("Calling rtModelExecuteAsync use time: %" PRIu64 " ns", endTime - beginTime);
    return ret;
}

RUNTIME_HOOK_DEFINE(rtsModelExecute, rtModel_t mdl, int32_t timeout)
{
    atomic_fetch_add(&hasModelExecuteSync, 1);
    core_limiter(NULL, NULL, NULL);
    uint64_t beginTime = ns_now();
    LOG_DEBUG("Calling rtsModelExecute at time %" PRIu64 "", beginTime);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsModelExecute, mdl, timeout);
    uint64_t endTime = ns_now();
    LOG_DEBUG("Calling rtsModelExecute use time: %" PRIu64 " ns", endTime - beginTime);
    if (ret == ACL_RT_SUCCESS) {
        atomic_fetch_sub(&hasModelExecuteSync, 1);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtModelExecuteSync, rtModel_t mdl, rtStream_t stm, uint32_t flag, int32_t timeout)
{
    atomic_fetch_add(&hasModelExecuteSync, 1);
    core_limiter(stm, NULL, NULL);
    uint64_t beginTime = ns_now();
    LOG_DEBUG("Calling rtModelExecuteSync at time %" PRIu64 "", beginTime);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtModelExecuteSync, mdl, stm, flag, timeout);
    uint64_t endTime = ns_now();
    LOG_DEBUG("Calling rtModelExecuteSync use time: %" PRIu64 " ns", endTime - beginTime);
    if (ret == ACL_RT_SUCCESS) {
        atomic_fetch_sub(&hasModelExecuteSync, 1);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtStreamBeginCapture, rtStream_t stm, const rtStreamCaptureMode mode)
{
    bool capture = true;
    core_limiter(stm, set_stream_capture, &capture);
    uint64_t beginTime = ns_now();
    LOG_DEBUG("Stream %p calling rtStreamBeginCapture at time %" PRIu64 "", (void*)stm, beginTime);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtStreamBeginCapture, stm, mode);
    uint64_t endTime = ns_now();
    LOG_DEBUG("Calling rtStreamBeginCapture use time: %" PRIu64 " ns", endTime - beginTime);
    return ret;
}

RUNTIME_HOOK_DEFINE(rtStreamEndCapture, rtStream_t stm, rtModel_t *captureMdl)
{
    core_limiter(stm, NULL, NULL);
    uint64_t beginTime = ns_now();
    LOG_DEBUG("Stream %p calling rtStreamEndCapture at time %" PRIu64 "", (void*)stm, beginTime);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtStreamEndCapture, stm, captureMdl);
    uint64_t endTime = ns_now();
    if (ret == ACL_RT_SUCCESS) {
        bool capture = false;
        set_stream_capture(&capture, stm);
    }
    LOG_DEBUG("Calling rtStreamEndCapture use time: %" PRIu64 " ns", endTime - beginTime);
    return ret;
}