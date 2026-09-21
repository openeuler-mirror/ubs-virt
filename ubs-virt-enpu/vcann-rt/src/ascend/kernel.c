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
#include "rts_kernel.h"
#include "rts_model.h"
#include "rts_stars.h"
#include "runtime_hook.h"
#include "vnpu_stats.h"

#ifndef RT_STUB_RT_DIM3_DEFINED
#ifndef rtDim3
#define rtDim3 rtdim3
#endif

typedef struct rtDim3 {
    uint32_t x;
    uint32_t y;
    uint32_t z;
} rtDim3;

#ifndef dim3
#define dim3 aclrtDim3 // 旧版本用 aclrtDim3 作为别名
#endif

typedef struct dim3 {
    uint32_t x;
    uint32_t y;
    uint32_t z;
} dim3;
#endif

RUNTIME_HOOK_DEFINE(rtKernelLaunch, const void *stubFunc, uint32_t blockDim, void *args, uint32_t argsSize,
                    rtSmDesc_t *smDesc, rtStream_t stm)
{
    LOG_DEBUG("Hook init rtKernelLaunch.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, blockDim);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret =
            RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunch, stubFunc, blockDim, args, argsSize, smDesc, stm);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunch, stubFunc, blockDim, args, argsSize, smDesc, stm);
}

RUNTIME_HOOK_DEFINE(rtKernelLaunchWithHandle, void *hdl, const uint64_t tilingKey, uint32_t blockDim,
                    rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, const void *kernelInfo)
{
    LOG_DEBUG("Hook init rtKernelLaunchWithHandle.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, blockDim);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunchWithHandle, hdl, tilingKey, blockDim, argsInfo,
                                         smDesc, stm, kernelInfo);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunchWithHandle, hdl, tilingKey, blockDim, argsInfo, smDesc,
                             stm, kernelInfo);
}

RUNTIME_HOOK_DEFINE(rtKernelLaunchWithHandleV2, void *hdl, const uint64_t tilingKey, uint32_t blockDim,
                    rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, const rtTaskCfgInfo_t *cfgInfo)
{
    LOG_DEBUG("Hook init rtKernelLaunchWithHandleV2.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, blockDim);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunchWithHandleV2, hdl, tilingKey, blockDim,
                                         argsInfo, smDesc, stm, cfgInfo);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunchWithHandleV2, hdl, tilingKey, blockDim, argsInfo, smDesc,
                             stm, cfgInfo);
}

RUNTIME_HOOK_DEFINE(rtKernelLaunchWithFlag, const void *stubFunc, uint32_t blockDim, rtArgsEx_t *argsInfo,
                    rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags)
{
    LOG_DEBUG("Hook init rtKernelLaunchWithFlag.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, blockDim);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunchWithFlag, stubFunc, blockDim, argsInfo, smDesc,
                                         stm, flags);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunchWithFlag, stubFunc, blockDim, argsInfo, smDesc, stm,
                             flags);
}

RUNTIME_HOOK_DEFINE(rtKernelLaunchWithFlagV2, const void *stubFunc, uint32_t blockDim, rtArgsEx_t *argsInfo,
                    rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags, const rtTaskCfgInfo_t *cfgInfo)
{
    LOG_DEBUG("Hook init rtKernelLaunchWithFlagV2.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, blockDim);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunchWithFlagV2, stubFunc, blockDim, argsInfo,
                                         smDesc, stm, flags, cfgInfo);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunchWithFlagV2, stubFunc, blockDim, argsInfo, smDesc, stm,
                             flags, cfgInfo);
}

RUNTIME_HOOK_DEFINE(rtKernelLaunchEx, void *args, uint32_t argsSize, uint32_t flags, rtStream_t stm)
{
    LOG_DEBUG("Hook init rtKernelLaunchEx.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, get_aicore_num());
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunchEx, args, argsSize, flags, stm);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunchEx, args, argsSize, flags, stm);
}

RUNTIME_HOOK_DEFINE(rtKernelLaunchFwk, const char_t *opName, void *args, uint32_t argsSize, uint32_t flags,
                    rtStream_t rtStream)
{
    LOG_DEBUG("Hook init rtKernelLaunchFwk.");
    core_limiter(rtStream, NULL, NULL);
    launch_stats_dispatch(rtStream, get_aicore_num());
    if (is_random_sampling()) {
        sampling_begin(rtStream);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunchFwk, opName, args, argsSize, flags, rtStream);
        sampling_end(rtStream);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtKernelLaunchFwk, opName, args, argsSize, flags, rtStream);
}

RUNTIME_HOOK_DEFINE(rtCpuKernelLaunchWithFlag, const void *soName, const void *kernelName, uint32_t blockDim,
                    const rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags)
{
    LOG_DEBUG("Hook init rtCpuKernelLaunchWithFlag.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, blockDim);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtCpuKernelLaunchWithFlag, soName, kernelName, blockDim,
                                         argsInfo, smDesc, stm, flags);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtCpuKernelLaunchWithFlag, soName, kernelName, blockDim, argsInfo,
                             smDesc, stm, flags);
}

RUNTIME_HOOK_DEFINE(rtAicpuKernelLaunchWithFlag, const rtKernelLaunchNames_t *launchNames, uint32_t blockDim,
                    const rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags)
{
    LOG_DEBUG("Hook init rtAicpuKernelLaunchWithFlag.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, blockDim);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtAicpuKernelLaunchWithFlag, launchNames, blockDim, argsInfo,
                                         smDesc, stm, flags);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtAicpuKernelLaunchWithFlag, launchNames, blockDim, argsInfo, smDesc,
                             stm, flags);
}

RUNTIME_HOOK_DEFINE(rtAicpuKernelLaunchExWithArgs, const uint32_t kernelType, const char_t *const opName,
                    const uint32_t blockDim, const rtAicpuArgsEx_t *argsInfo, rtSmDesc_t *const smDesc,
                    const rtStream_t stm, const uint32_t flags)
{
    LOG_DEBUG("Hook init rtAicpuKernelLaunchExWithArgs.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, blockDim);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtAicpuKernelLaunchExWithArgs, kernelType, opName, blockDim,
                                         argsInfo, smDesc, stm, flags);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtAicpuKernelLaunchExWithArgs, kernelType, opName, blockDim, argsInfo,
                             smDesc, stm, flags);
}

RUNTIME_HOOK_DEFINE(rtLaunchKernelByFuncHandle, rtFuncHandle funcHandle, uint32_t blockDim,
                    rtLaunchArgsHandle argsHandle, rtStream_t stm)
{
    LOG_DEBUG("Hook init rtLaunchKernelByFuncHandle.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, blockDim);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret =
            RUNTIME_HOOK_CALL(rt_library_entry, rtLaunchKernelByFuncHandle, funcHandle, blockDim, argsHandle, stm);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtLaunchKernelByFuncHandle, funcHandle, blockDim, argsHandle, stm);
}

RUNTIME_HOOK_DEFINE(rtLaunchKernelByFuncHandleV2, rtFuncHandle funcHandle, uint32_t blockDim,
                    rtLaunchArgsHandle argsHandle, rtStream_t stm, const rtTaskCfgInfo_t *cfgInfo)
{
    LOG_DEBUG("Hook init rtLaunchKernelByFuncHandleV2.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, blockDim);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtLaunchKernelByFuncHandleV2, funcHandle, blockDim,
                                         argsHandle, stm, cfgInfo);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtLaunchKernelByFuncHandleV2, funcHandle, blockDim, argsHandle, stm,
                             cfgInfo);
}

RUNTIME_HOOK_DEFINE(rtLaunchKernelByFuncHandleV3, rtFuncHandle funcHandle, uint32_t blockDim,
                    const rtArgsEx_t *const argsInfo, rtStream_t stm, const rtTaskCfgInfo_t *const cfgInfo)
{
    LOG_DEBUG("Hook init rtLaunchKernelByFuncHandleV3.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, blockDim);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtLaunchKernelByFuncHandleV3, funcHandle, blockDim, argsInfo,
                                         stm, cfgInfo);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtLaunchKernelByFuncHandleV3, funcHandle, blockDim, argsInfo, stm,
                             cfgInfo);
}

RUNTIME_HOOK_DEFINE(aclrtLaunchKernelImpl, aclrtFuncHandle funcHandle, uint32_t numBlocks, const void *argsData,
                    size_t argsSize, aclrtStream stream)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchKernelImpl, funcHandle, numBlocks, argsData, argsSize,
                                 stream);
    }
    LOG_DEBUG("Hook init aclrtLaunchKernelImpl.");
    core_limiter(stream, NULL, NULL);
    launch_stats_dispatch(stream, numBlocks);
    if (is_random_sampling()) {
        sampling_begin(stream);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchKernelImpl, funcHandle, numBlocks, argsData,
                                         argsSize, stream);
        sampling_end(stream);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchKernelImpl, funcHandle, numBlocks, argsData, argsSize,
                             stream);
}

RUNTIME_HOOK_DEFINE(rtVectorCoreKernelLaunchWithHandle, void *hdl, const uint64_t tilingKey, uint32_t blockDim,
                    rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, const rtTaskCfgInfo_t *cfgInfo)
{
    LOG_DEBUG("Hook init rtVectorCoreKernelLaunchWithHandle.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, blockDim);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtVectorCoreKernelLaunchWithHandle, hdl, tilingKey, blockDim,
                                         argsInfo, smDesc, stm, cfgInfo);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtVectorCoreKernelLaunchWithHandle, hdl, tilingKey, blockDim, argsInfo,
                             smDesc, stm, cfgInfo);
}

RUNTIME_HOOK_DEFINE(rtVectorCoreKernelLaunch, const void *stubFunc, uint32_t blockDim, rtArgsEx_t *argsInfo,
                    rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags, const rtTaskCfgInfo_t *cfgInfo)
{
    LOG_DEBUG("Hook init rtVectorCoreKernelLaunch.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, blockDim);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtVectorCoreKernelLaunch, stubFunc, blockDim, argsInfo,
                                         smDesc, stm, flags, cfgInfo);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtVectorCoreKernelLaunch, stubFunc, blockDim, argsInfo, smDesc, stm,
                             flags, cfgInfo);
}

RUNTIME_HOOK_DEFINE(rtsLaunchKernelWithHostArgs, rtFuncHandle funcHandle, uint32_t numBlocks, rtStream_t stm,
                    rtKernelLaunchCfg_t *cfg, void *hostArgs, uint32_t argsSize, rtPlaceHolderInfo_t *placeHolderArray,
                    uint32_t placeHolderNum)
{
    LOG_DEBUG("Hook init rtsLaunchKernelWithHostArgs.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, numBlocks);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsLaunchKernelWithHostArgs, funcHandle, numBlocks, stm, cfg,
                                         hostArgs, argsSize, placeHolderArray, placeHolderNum);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtsLaunchKernelWithHostArgs, funcHandle, numBlocks, stm, cfg, hostArgs,
                             argsSize, placeHolderArray, placeHolderNum);
}

RUNTIME_HOOK_DEFINE(aclrtLaunchKernelWithHostArgsImpl, aclrtFuncHandle funcHandle, uint32_t numBlocks,
                    aclrtStream stream, aclrtLaunchKernelCfg *cfg, void *hostArgs, size_t argsSize,
                    aclrtPlaceHolderInfo *placeHolderArray, size_t placeHolderNum)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchKernelWithHostArgsImpl, funcHandle, numBlocks, stream,
                                 cfg, hostArgs, argsSize, placeHolderArray, placeHolderNum);
    }
    LOG_DEBUG("Hook init aclrtLaunchKernelWithHostArgsImpl.");
    core_limiter(stream, NULL, NULL);
    launch_stats_dispatch(stream, numBlocks);
    if (is_random_sampling()) {
        sampling_begin(stream);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchKernelWithHostArgsImpl, funcHandle, numBlocks,
                                         stream, cfg, hostArgs, argsSize, placeHolderArray, placeHolderNum);
        sampling_end(stream);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchKernelWithHostArgsImpl, funcHandle, numBlocks, stream, cfg,
                             hostArgs, argsSize, placeHolderArray, placeHolderNum);
}

RUNTIME_HOOK_DEFINE(rtsLaunchKernelWithConfig, rtFuncHandle funcHandle, uint32_t numBlocks, rtStream_t stm,
                    rtKernelLaunchCfg_t *cfg, rtArgsHandle argsHandle, void *reserve)
{
    LOG_DEBUG("Hook init rtsLaunchKernelWithConfig.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, numBlocks);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsLaunchKernelWithConfig, funcHandle, numBlocks, stm, cfg,
                                         argsHandle, reserve);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtsLaunchKernelWithConfig, funcHandle, numBlocks, stm, cfg, argsHandle,
                             reserve);
}

RUNTIME_HOOK_DEFINE(aclrtLaunchKernelWithConfigImpl, aclrtFuncHandle funcHandle, uint32_t numBlocks, aclrtStream stream,
                    aclrtLaunchKernelCfg *cfg, aclrtArgsHandle argsHandle, void *reserve)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchKernelWithConfigImpl, funcHandle, numBlocks, stream, cfg,
                                 argsHandle, reserve);
    }
    LOG_DEBUG("Hook init aclrtLaunchKernelWithConfigImpl.");
    core_limiter(stream, NULL, NULL);
    launch_stats_dispatch(stream, numBlocks);
    if (is_random_sampling()) {
        sampling_begin(stream);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchKernelWithConfigImpl, funcHandle, numBlocks,
                                         stream, cfg, argsHandle, reserve);
        sampling_end(stream);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchKernelWithConfigImpl, funcHandle, numBlocks, stream, cfg,
                             argsHandle, reserve);
}

RUNTIME_HOOK_DEFINE(rtsLaunchKernelWithDevArgs, rtFuncHandle funcHandle, uint32_t numBlocks, rtStream_t stm,
                    rtKernelLaunchCfg_t *cfg, const void *args, uint32_t argsSize, void *reserve)
{
    LOG_DEBUG("Hook init rtsLaunchKernelWithDevArgs.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, numBlocks);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsLaunchKernelWithDevArgs, funcHandle, numBlocks, stm, cfg,
                                         args, argsSize, reserve);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtsLaunchKernelWithDevArgs, funcHandle, numBlocks, stm, cfg, args,
                             argsSize, reserve);
}

RUNTIME_HOOK_DEFINE(aclrtLaunchKernelV2Impl, aclrtFuncHandle funcHandle, uint32_t numBlocks, const void *argsData,
                    size_t argsSize, aclrtLaunchKernelCfg *cfg, aclrtStream stream)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchKernelV2Impl, funcHandle, numBlocks, argsData, argsSize,
                                 cfg, stream);
    }
    LOG_DEBUG("Hook init aclrtLaunchKernelV2Impl.");
    core_limiter(stream, NULL, NULL);
    launch_stats_dispatch(stream, numBlocks);
    if (is_random_sampling()) {
        sampling_begin(stream);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchKernelV2Impl, funcHandle, numBlocks, argsData,
                                         argsSize, cfg, stream);
        sampling_end(stream);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchKernelV2Impl, funcHandle, numBlocks, argsData, argsSize, cfg,
                             stream);
}

RUNTIME_HOOK_DEFINE(rtsLaunchRandomNumTask, const rtRandomNumTaskInfo_t *taskInfo, const rtStream_t stm, void *reserve)
{
    LOG_DEBUG("Hook init rtsLaunchRandomNumTask.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, get_aicore_num());
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsLaunchRandomNumTask, taskInfo, stm, reserve);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtsLaunchRandomNumTask, taskInfo, stm, reserve);
}

RUNTIME_HOOK_DEFINE(aclrtRandomNumAsyncImpl, const aclrtRandomNumTaskInfo *taskInfo, const aclrtStream stream,
                    void *reserve)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtRandomNumAsyncImpl, taskInfo, stream, reserve);
    }
    LOG_DEBUG("Hook init aclrtRandomNumAsyncImpl.");
    core_limiter(stream, NULL, NULL);
    launch_stats_dispatch(stream, get_aicore_num());
    if (is_random_sampling()) {
        sampling_begin(stream);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, aclrtRandomNumAsyncImpl, taskInfo, stream, reserve);
        sampling_end(stream);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtRandomNumAsyncImpl, taskInfo, stream, reserve);
}

RUNTIME_HOOK_DEFINE(rtsLaunchReduceAsyncTask, const rtReduceInfo_t *reduceInfo, const rtStream_t stm,
                    const void *reserve)
{
    LOG_DEBUG("Hook init rtsLaunchReduceAsyncTask.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, get_aicore_num());
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsLaunchReduceAsyncTask, reduceInfo, stm, reserve);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtsLaunchReduceAsyncTask, reduceInfo, stm, reserve);
}

RUNTIME_HOOK_DEFINE(aclrtReduceAsyncImpl, void *dst, const void *src, uint64_t count, aclrtReduceKind kind,
                    aclDataType type, aclrtStream stream, void *reserve)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtReduceAsyncImpl, dst, src, count, kind, type, stream, reserve);
    }
    LOG_DEBUG("Hook init aclrtReduceAsyncImpl.");
    core_limiter(stream, NULL, NULL);
    launch_stats_dispatch(stream, get_aicore_num());
    if (is_random_sampling()) {
        sampling_begin(stream);
        aclError ret =
            RUNTIME_HOOK_CALL(rt_library_entry, aclrtReduceAsyncImpl, dst, src, count, kind, type, stream, reserve);
        sampling_end(stream);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtReduceAsyncImpl, dst, src, count, kind, type, stream, reserve);
}

RUNTIME_HOOK_DEFINE(rtsLaunchUpdateTask, rtStream_t destStm, uint32_t destTaskId, rtStream_t stm,
                    rtTaskUpdateCfg_t *cfg)
{
    LOG_DEBUG("Hook init rtsLaunchUpdateTask.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, get_aicore_num());
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtsLaunchUpdateTask, destStm, destTaskId, stm, cfg);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtsLaunchUpdateTask, destStm, destTaskId, stm, cfg);
}

RUNTIME_HOOK_DEFINE(aclrtTaskUpdateAsyncImpl, aclrtStream taskStream, uint32_t taskId, aclrtTaskUpdateInfo *info,
                    aclrtStream execStream)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtTaskUpdateAsyncImpl, taskStream, taskId, info, execStream);
    }
    LOG_DEBUG("Hook init aclrtTaskUpdateAsyncImpl.");
    core_limiter(execStream, NULL, NULL);
    launch_stats_dispatch(execStream, get_aicore_num());
    if (is_random_sampling()) {
        sampling_begin(execStream);
        aclError ret =
            RUNTIME_HOOK_CALL(rt_library_entry, aclrtTaskUpdateAsyncImpl, taskStream, taskId, info, execStream);
        sampling_end(execStream);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtTaskUpdateAsyncImpl, taskStream, taskId, info, execStream);
}

RUNTIME_HOOK_DEFINE(rtLaunchKernelWithArgsArray, void *func, uint32_t numBlocks, rtStream_t stm,
                    rtKernelLaunchCfg_t *cfg, void **args)
{
    LOG_DEBUG("Hook init rtLaunchKernelWithArgsArray.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, numBlocks);
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret =
            RUNTIME_HOOK_CALL(rt_library_entry, rtLaunchKernelWithArgsArray, func, numBlocks, stm, cfg, args);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtLaunchKernelWithArgsArray, func, numBlocks, stm, cfg, args);
}

RUNTIME_HOOK_DEFINE(aclrtLaunchKernelWithArgsArrayImpl, void *func, uint32_t numBlocks, aclrtStream stream,
                    aclrtLaunchKernelCfg *cfg, void **args)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchKernelWithArgsArrayImpl, func, numBlocks, stream, cfg,
                                 args);
    }
    LOG_DEBUG("Hook init aclrtLaunchKernelWithArgsArrayImpl.");
    core_limiter(stream, NULL, NULL);
    launch_stats_dispatch(stream, numBlocks);
    if (is_random_sampling()) {
        sampling_begin(stream);
        aclError ret =
            RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchKernelWithArgsArrayImpl, func, numBlocks, stream, cfg, args);
        sampling_end(stream);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchKernelWithArgsArrayImpl, func, numBlocks, stream, cfg, args);
}

RUNTIME_HOOK_DEFINE(rtLaunchSIMTKernelWithHostArgs, void *func, rtDim3 gridDim, rtDim3 blockDim, size_t dynUbufSize,
                    rtStream_t stm, rtKernelLaunchCfg_t *cfg, void *hostArgs, uint32_t argsSize,
                    rtPlaceHolderInfo_t *placeHolderArray, uint32_t placeHolderNum)
{
    LOG_DEBUG("Hook init rtLaunchSIMTKernelWithHostArgs.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, get_aicore_num());
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtLaunchSIMTKernelWithHostArgs, func, gridDim, blockDim,
                                         dynUbufSize, stm, cfg, hostArgs, argsSize, placeHolderArray, placeHolderNum);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtLaunchSIMTKernelWithHostArgs, func, gridDim, blockDim, dynUbufSize,
                             stm, cfg, hostArgs, argsSize, placeHolderArray, placeHolderNum);
}

RUNTIME_HOOK_DEFINE(aclrtLaunchSIMTKernelWithHostArgsImpl, void *func, dim3 gridDim, dim3 blockDim, size_t dynUbufSize,
                    aclrtStream stream, aclrtLaunchKernelCfg *cfg, void *hostArgs, size_t argsSize,
                    aclrtPlaceHolderInfo *placeHolderArray, size_t placeHolderNum)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchSIMTKernelWithHostArgsImpl, func, gridDim, blockDim,
                                 dynUbufSize, stream, cfg, hostArgs, argsSize, placeHolderArray, placeHolderNum);
    }
    LOG_DEBUG("Hook init aclrtLaunchSIMTKernelWithHostArgsImpl.");
    core_limiter(stream, NULL, NULL);
    launch_stats_dispatch(stream, get_aicore_num());
    if (is_random_sampling()) {
        sampling_begin(stream);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchSIMTKernelWithHostArgsImpl, func, gridDim,
                                         blockDim, dynUbufSize, stream, cfg, hostArgs, argsSize, placeHolderArray,
                                         placeHolderNum);
        sampling_end(stream);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchSIMTKernelWithHostArgsImpl, func, gridDim, blockDim,
                             dynUbufSize, stream, cfg, hostArgs, argsSize, placeHolderArray, placeHolderNum);
}

RUNTIME_HOOK_DEFINE(rtLaunchSIMTKernelWithArgsArray, void *func, rtDim3 gridDim, rtDim3 blockDim, size_t dynUbufSize,
                    rtStream_t stm, rtKernelLaunchCfg_t *cfg, void **args)
{
    LOG_DEBUG("Hook init rtLaunchSIMTKernelWithArgsArray.");
    core_limiter(stm, NULL, NULL);
    launch_stats_dispatch(stm, get_aicore_num());
    if (is_random_sampling()) {
        sampling_begin(stm);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, rtLaunchSIMTKernelWithArgsArray, func, gridDim, blockDim,
                                         dynUbufSize, stm, cfg, args);
        sampling_end(stm);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtLaunchSIMTKernelWithArgsArray, func, gridDim, blockDim, dynUbufSize,
                             stm, cfg, args);
}

RUNTIME_HOOK_DEFINE(aclrtLaunchSIMTKernelWithArgsArrayImpl, void *func, dim3 gridDim, dim3 blockDim, size_t dynUbufSize,
                    aclrtStream stream, aclrtLaunchKernelCfg *cfg, void **args)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchSIMTKernelWithArgsArrayImpl, func, gridDim, blockDim,
                                 dynUbufSize, stream, cfg, args);
    }
    LOG_DEBUG("Hook init aclrtLaunchSIMTKernelWithArgsArrayImpl.");
    core_limiter(stream, NULL, NULL);
    launch_stats_dispatch(stream, get_aicore_num());
    if (is_random_sampling()) {
        sampling_begin(stream);
        aclError ret = RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchSIMTKernelWithArgsArrayImpl, func, gridDim,
                                         blockDim, dynUbufSize, stream, cfg, args);
        sampling_end(stream);
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtLaunchSIMTKernelWithArgsArrayImpl, func, gridDim, blockDim,
                             dynUbufSize, stream, cfg, args);
}