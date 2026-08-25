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

#include "core_limiter.h"
#include "log.h"
#include "npu_manager.h"
#include "runtime_hook.h"
#include "vnpu_stats.h"

static void record_model_execute_stats(rtModel_t mdl)
{
    uint64_t block_dim = 0;
    uint64_t count = 0;
    if (model_stats_get(mdl, &block_dim, &count) == 0 && count > 0) {
        vnpu_stats_record(get_vnpu_id(), (uint32_t)block_dim, count);
    }
}

static void track_sync_model(void *param, rtStream_t stream)
{
    (void)stream;
    atomic_fetch_add(&hasModelExecuteSync, 1);
    *(bool *)param = true;
}

RUNTIME_HOOK_DEFINE(rtModelExecute, rtModel_t mdl, rtStream_t stm, uint32_t flag)
{
    core_limiter(stm, NULL, NULL);
    record_model_execute_stats(mdl);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtModelExecute, mdl, stm, flag);
        sampling_graph_end(stm, mdl);
        return ret;
    }
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtModelExecute, mdl, stm, flag);
    return ret;
}

RUNTIME_HOOK_DEFINE(rtModelExecuteAsync, rtModel_t mdl, rtStream_t stm, uint32_t flag)
{
    core_limiter(stm, NULL, NULL);
    record_model_execute_stats(mdl);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtModelExecuteAsync, mdl, stm, flag);
        sampling_graph_end(stm, mdl);
        return ret;
    }
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtModelExecuteAsync, mdl, stm, flag);
    return ret;
}

RUNTIME_HOOK_DEFINE(rtsModelExecute, rtModel_t mdl, int32_t timeout)
{
    bool tracked = false;
    core_limiter(NULL, track_sync_model, &tracked);
    record_model_execute_stats(mdl);
    uint64_t begin_time = ns_now();
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsModelExecute, mdl, timeout);
    uint64_t end_time = ns_now();
    int rc = add_sample_record(get_kernel_count(mdl), end_time - begin_time, end_time);
    CHECK_ERROR_CODE(rc, "Add rtsModelExecute sample record failed.");
    if (tracked) {
        atomic_fetch_sub(&hasModelExecuteSync, 1);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtModelExecuteSync, rtModel_t mdl, rtStream_t stm, uint32_t flag, int32_t timeout)
{
    bool tracked = false;
    core_limiter(stm, track_sync_model, &tracked);
    record_model_execute_stats(mdl);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtModelExecuteSync, mdl, stm, flag, timeout);
        sampling_graph_end(stm, mdl);
        if (tracked) {
            atomic_fetch_sub(&hasModelExecuteSync, 1);
        }
        return ret;
    }
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtModelExecuteSync, mdl, stm, flag, timeout);
    if (tracked) {
        atomic_fetch_sub(&hasModelExecuteSync, 1);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtStreamBeginCapture, rtStream_t stm, const rtStreamCaptureMode mode)
{
    bool capture = true;
    core_limiter(stm, set_stream_capture, &capture);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtStreamBeginCapture, stm, mode);
    return ret;
}

RUNTIME_HOOK_DEFINE(rtStreamEndCapture, rtStream_t stm, rtModel_t *captureMdl)
{
    core_limiter(stm, NULL, NULL);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtStreamEndCapture, stm, captureMdl);
    if (ret == ACL_RT_SUCCESS && is_core_limit()) {
        bool capture = false;
        set_stream_capture(&capture, stm);
        if (captureMdl != NULL && *captureMdl != NULL) {
            capture_stats_transfer_to_model(stm, *captureMdl);
        }
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtsModelExecuteAsync, rtModel_t mdl, rtStream_t stm)
{
    core_limiter(stm, NULL, NULL);
    record_model_execute_stats(mdl);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsModelExecuteAsync, mdl, stm);
        sampling_graph_end(stm, mdl);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtsModelExecuteAsync, mdl, stm);
}