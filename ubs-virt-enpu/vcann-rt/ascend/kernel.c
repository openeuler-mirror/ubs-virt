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

RUNTIME_HOOK_DEFINE(rtKernelLaunch, const void *stubFunc, uint32_t blockDim,
    void *args, uint32_t argsSize, rtSmDesc_t *smDesc, rtStream_t stm)
{
    core_limiter(stm, NULL, NULL);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunch, stubFunc, blockDim, args, argsSize, smDesc, stm);
}

RUNTIME_HOOK_DEFINE(rtKernelLaunchWithHandle, void *hdl, const uint64_t tilingKey, uint32_t blockDim,
    rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, const void *kernelInfo)
{
    core_limiter(stm, NULL, NULL);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunchWithHandle, hdl, tilingKey, blockDim, argsInfo, smDesc,
        stm, kernelInfo);
}

RUNTIME_HOOK_DEFINE(rtKernelLaunchWithHandleV2, void *hdl, const uint64_t tilingKey, uint32_t blockDim,
    rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, const rtTaskCfgInfo_t *cfgInfo)
{
    core_limiter(stm, NULL, NULL);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunchWithHandleV2, hdl, tilingKey, blockDim, argsInfo, smDesc,
        stm, cfgInfo);
}

RUNTIME_HOOK_DEFINE(rtKernelLaunchWithFlag, const void *stubFunc, uint32_t blockDim, rtArgsEx_t *argsInfo,
    rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags)
{
    core_limiter(stm, NULL, NULL);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunchWithFlag, stubFunc, blockDim, argsInfo, smDesc, stm,
        flags);
}

RUNTIME_HOOK_DEFINE(rtKernelLaunchWithFlagV2, const void *stubFunc, uint32_t blockDim, rtArgsEx_t *argsInfo,
    rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags, const rtTaskCfgInfo_t *cfgInfo)
{
    core_limiter(stm, NULL, NULL);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunchWithFlagV2, stubFunc, blockDim, argsInfo, smDesc, stm,
        flags, cfgInfo);
}

RUNTIME_HOOK_DEFINE(rtKernelLaunchEx, void *args, uint32_t argsSize, uint32_t flags, rtStream_t stm)
{
    core_limiter(stm, NULL, NULL);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunchEx, args, argsSize, flags, stm);
}

RUNTIME_HOOK_DEFINE(rtKernelLaunchFwk, const char_t *opName, void *args, uint32_t argsSize, uint32_t flags,
    rtStream_t rtStream)
{
    core_limiter(rtStream, NULL, NULL);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunchFwk, opName, args, argsSize, flags, rtStream);
}

RUNTIME_HOOK_DEFINE(rtCpuKernelLaunch, const void *soName, const void *kernelName, uint32_t blockDim, const void *args,
    uint32_t argsSize, rtSmDesc_t *smDesc, rtStream_t stm)
{
    core_limiter(stm, NULL, NULL);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtCpuKernelLaunch, soName, kernelName, blockDim, args, argsSize, smDesc,
        stm);
}

RUNTIME_HOOK_DEFINE(rtCpuKernelLaunchWithFlag, const void *soName, const void *kernelName, uint32_t blockDim,
    const rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags)
{
    core_limiter(stm, NULL, NULL);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtCpuKernelLaunchWithFlag, soName, kernelName, blockDim, argsInfo,
        smDesc, stm, flags);
}

RUNTIME_HOOK_DEFINE(rtAicpuKernelLaunchWithFlag, const rtKernelLaunchNames_t *launchNames, uint32_t blockDim,
    const rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags)
{
    core_limiter(stm, NULL, NULL);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtAicpuKernelLaunchWithFlag, launchNames, blockDim, argsInfo, smDesc,
        stm, flags);
}

RUNTIME_HOOK_DEFINE(rtAicpuKernelLaunchExWithArgs, const uint32_t kernelType, const char_t *const opName,
    const uint32_t blockDim, const rtAicpuArgsEx_t *argsInfo, rtSmDesc_t *const smDesc,
    const rtStream_t stm, const uint32_t flags)
{
    core_limiter(stm, NULL, NULL);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtAicpuKernelLaunchExWithArgs, kernelType, opName, blockDim, argsInfo,
        smDesc, stm, flags);
}

RUNTIME_HOOK_DEFINE(rtLaunchKernelByFuncHandle, rtFuncHandle funcHandle, uint32_t blockDim,
    rtLaunchArgsHandle argsHandle, rtStream_t stm)
{
    core_limiter(stm, NULL, NULL);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtLaunchKernelByFuncHandle, funcHandle, blockDim, argsHandle, stm);
}

RUNTIME_HOOK_DEFINE(rtLaunchKernelByFuncHandleV2, rtFuncHandle funcHandle, uint32_t blockDim,
    rtLaunchArgsHandle argsHandle, rtStream_t stm, const rtTaskCfgInfo_t *cfgInfo)
{
    core_limiter(stm, NULL, NULL);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtLaunchKernelByFuncHandleV2, funcHandle, blockDim, argsHandle, stm,
        cfgInfo);
}

RUNTIME_HOOK_DEFINE(rtLaunchKernelByFuncHandleV3, rtFuncHandle funcHandle, uint32_t blockDim,
    const rtArgsEx_t *const argsInfo, rtStream_t stm, const rtTaskCfgInfo_t *const cfgInfo)
{
    core_limiter(stm, NULL, NULL);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtLaunchKernelByFuncHandleV3, funcHandle, blockDim, argsInfo, stm,
        cfgInfo);
}

RUNTIME_HOOK_DEFINE(rtVectorCoreKernelLaunchWithHandle, void *hdl, const uint64_t tilingKey, uint32_t blockDim,
    rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, const rtTaskCfgInfo_t *cfgInfo)
{
    core_limiter(stm, NULL, NULL);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtVectorCoreKernelLaunchWithHandle, hdl, tilingKey, blockDim, argsInfo,
        smDesc, stm, cfgInfo);
}

RUNTIME_HOOK_DEFINE(rtVectorCoreKernelLaunch, const void *stubFunc, uint32_t blockDim, rtArgsEx_t *argsInfo,
    rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags, const rtTaskCfgInfo_t *cfgInfo)
{
    core_limiter(stm, NULL, NULL);
    return RUNTIME_HOOK_CALL(rt_library_entry, rtVectorCoreKernelLaunch, stubFunc, blockDim, argsInfo, smDesc, stm,
        flags, cfgInfo);
}