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

RUNTIME_HOOK_DEFINE(rtFftsPlusTaskLaunch, rtFftsPlusTaskInfo_t *fftsPlusTaskInfo, rtStream_t stm)
{
    LOG_DEBUG("Hook init rtFftsPlusTaskLaunch.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, get_aicore_num());
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtFftsPlusTaskLaunch, fftsPlusTaskInfo, stm);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtFftsPlusTaskLaunch, fftsPlusTaskInfo, stm);
}

RUNTIME_HOOK_DEFINE(rtFftsPlusTaskLaunchWithFlag, rtFftsPlusTaskInfo_t *fftsPlusTaskInfo, rtStream_t stm, uint32_t flag)
{
    LOG_DEBUG("Hook init rtFftsPlusTaskLaunchWithFlag.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, get_aicore_num());
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtFftsPlusTaskLaunchWithFlag, fftsPlusTaskInfo, stm, flag);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtFftsPlusTaskLaunchWithFlag, fftsPlusTaskInfo, stm, flag);
}

RUNTIME_HOOK_DEFINE(rtStarsTaskLaunch, const void *taskSqe, uint32_t sqeLen, rtStream_t stm)
{
    LOG_DEBUG("Hook init rtStarsTaskLaunch.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, get_aicore_num());
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtStarsTaskLaunch, taskSqe, sqeLen, stm);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtStarsTaskLaunch, taskSqe, sqeLen, stm);
}

RUNTIME_HOOK_DEFINE(rtStarsTaskLaunchWithFlag, const void *taskSqe, uint32_t sqeLen, rtStream_t stm, uint32_t flag)
{
    LOG_DEBUG("Hook init rtStarsTaskLaunchWithFlag.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, get_aicore_num());
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtStarsTaskLaunchWithFlag, taskSqe, sqeLen, stm, flag);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtStarsTaskLaunchWithFlag, taskSqe, sqeLen, stm, flag);
}

RUNTIME_HOOK_DEFINE(rtMultipleTaskInfoLaunch, const void *taskInfo, rtStream_t stm)
{
    LOG_DEBUG("Hook init rtMultipleTaskInfoLaunch.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, get_aicore_num());
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtMultipleTaskInfoLaunch, taskInfo, stm);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtMultipleTaskInfoLaunch, taskInfo, stm);
}

RUNTIME_HOOK_DEFINE(rtsStreamBeginTaskGrp, rtStream_t stm)
{
    LOG_DEBUG("Hook init rtsStreamBeginTaskGrp.");
    core_limiter(stm, NULL, NULL);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsStreamBeginTaskGrp, stm);
    if (ret == ACL_RT_SUCCESS) {
        task_grp_begin(stm);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(aclmdlRICaptureTaskGrpBeginImpl, aclrtStream stream)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclmdlRICaptureTaskGrpBeginImpl, stream);
    }
    LOG_DEBUG("Hook init aclmdlRICaptureTaskGrpBeginImpl.");
    core_limiter(stream, NULL, NULL);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, aclmdlRICaptureTaskGrpBeginImpl, stream);
    if (ret == ACL_RT_SUCCESS) {
        task_grp_begin(stream);
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtsStreamEndTaskGrp, rtStream_t stm, rtTaskGrp_t *handle)
{
    LOG_DEBUG("Hook init rtsStreamEndTaskGrp.");
    core_limiter(stm, NULL, NULL);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsStreamEndTaskGrp, stm, handle);
    if (ret == ACL_RT_SUCCESS && handle != NULL && *handle != NULL) {
        task_grp_end(stm, *handle); /* group blockDim 确定，转移到 capture_stats_map */
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(aclmdlRICaptureTaskGrpEndImpl, aclrtStream stream, aclrtTaskGrp *handle)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclmdlRICaptureTaskGrpEndImpl, stream, handle);
    }
    LOG_DEBUG("Hook init aclmdlRICaptureTaskGrpEndImpl.");
    core_limiter(stream, NULL, NULL);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, aclmdlRICaptureTaskGrpEndImpl, stream, handle);
    if (ret == ACL_RT_SUCCESS && handle != NULL && *handle != NULL) {
        task_grp_end(stream, *handle); /* group blockDim 确定，转移到 capture_stats_map */
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtsStreamBeginTaskUpdate, rtStream_t stm, rtTaskGrp_t handle)
{
    LOG_DEBUG("Hook init rtsStreamBeginTaskUpdate.");
    core_limiter(stm, NULL, NULL);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsStreamBeginTaskUpdate, stm, handle);
    if (ret == ACL_RT_SUCCESS) {
        task_update_begin(stm, handle); /* 后续 kernellaunch 用于刷新该 group */
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(aclmdlRICaptureTaskUpdateBeginImpl, aclrtStream stream, aclrtTaskGrp handle)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclmdlRICaptureTaskUpdateBeginImpl, stream, handle);
    }
    LOG_DEBUG("Hook init aclmdlRICaptureTaskUpdateBeginImpl.");
    core_limiter(stream, NULL, NULL);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, aclmdlRICaptureTaskUpdateBeginImpl, stream, handle);
    if (ret == ACL_RT_SUCCESS) {
        task_update_begin(stream, handle); /* 后续 kernellaunch 用于刷新该 group */
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(rtsStreamEndTaskUpdate, rtStream_t stm)
{
    LOG_DEBUG("Hook init rtsStreamEndTaskUpdate.");
    core_limiter(stm, NULL, NULL);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsStreamEndTaskUpdate, stm);
    if (ret == ACL_RT_SUCCESS) {
        task_update_end(stm); /* 刷新覆盖 group 旧值 */
    }
    return ret;
}

RUNTIME_HOOK_DEFINE(aclmdlRICaptureTaskUpdateEndImpl, aclrtStream stream)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclmdlRICaptureTaskUpdateEndImpl, stream);
    }
    LOG_DEBUG("Hook init aclmdlRICaptureTaskUpdateEndImpl.");
    core_limiter(stream, NULL, NULL);
    aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, aclmdlRICaptureTaskUpdateEndImpl, stream);
    if (ret == ACL_RT_SUCCESS) {
        task_update_end(stream); /* 刷新覆盖 group 旧值 */
    }
    return ret;
}
