/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-ovs is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef RT_H
#define RT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef char char_t;
typedef int32_t rtError_t;

static const int32_t RT_ERROR_NONE = 0;

typedef void *rtAicpuArgsEx_t;
typedef void *rtArgsEx_t;
typedef void *rtBarrierTaskInfo_t;
typedef void *rtCmoOpCode_t;
typedef void *rtCmoTaskInfo_t;
typedef void *rtDrvMemProp_t;
typedef void *rtFftsPlusTaskInfo_t;
typedef void *rtFftsTaskInfo_t;
typedef void *rtKernelLaunchNames_t;
typedef void *rtMemType_t;
typedef void *rtModel_t;
typedef void *rtNotify_t;
typedef void *rtSmDesc_t;
typedef void *rtStream_t;
typedef void *rtEvent_t;
typedef void *rtTaskCfgInfo_t;
typedef void *rtDrvMemHandle;
typedef void *rtFuncHandle;
typedef void *rtLaunchArgsHandle;

typedef enum tagRtStreamCaptureMode {
    RT_STREAM_CAPTURE_MODE_GLOBAL = 0,
    RT_STREAM_CAPTURE_MODE_THREAD_LOCAL = 1,
    RT_STREAM_CAPTURE_MODE_RELAXED = 2,
    RT_STREAM_CAPTURE_MODE_MAX
} rtStreamCaptureMode;

typedef enum {
    RT_MEM_MALLOC_HUGE_FIRST,
    RT_MEM_MALLOC_HUGE_ONLY,
    RT_MEM_MALLOC_NORMAL_ONLY,
    RT_MEM_MALLOC_HUGE_FIRST_P2P,
    RT_MEM_MALLOC_HUGE_ONLY_P2P,
    RT_MEM_MALLOC_NORMAL_ONLY_P2P,
    RT_MEM_MALLOC_HUGE1G_ONLY,
    RT_MEM_MALLOC_HUGE1G_ONLY_P2P,
    RT_MEM_TYPE_LOW_BAND_WIDTH = 0x0100,
    RT_MEM_TYPE_HIGH_BAND_WIDTH = 0x1000,
    RT_MEM_ACCESS_USER_SPACE_READONLY = 0x100000,
} rtMallocPolicy;

typedef enum {
    RT_MEM_MALLOC_ATTR_RSV = 0,
    RT_MEM_MALLOC_ATTR_MODULE_ID,
    RT_MEM_MALLOC_ATTR_DEVICE_ID,
    RT_MEM_MALLOC_ATTR_VA_FLAG,
    RT_MEM_MALLOC_ATTR_MAX
} rtMallocAttr;

typedef union {
    uint16_t moduleId;
    uint32_t deviceId;
    uint32_t vaFlag;
    uint8_t rsv[8];
} rtMallocAttrValue;

typedef enum {
    RT_MEM_ADVISE_NONE = 0,
    RT_MEM_ADVISE_DVPP,
    RT_MEM_ADVISE_TS,
    RT_MEM_ADVISE_CACHED,
} rtMallocAdvise;

typedef struct {
    rtMallocAttr attr;
    rtMallocAttrValue value;
} rtMallocAttribute_t;

typedef struct {
    rtMallocAttribute_t *attrs;
    size_t numAttrs;
} rtMallocConfig_t;

#define RUNTIME_FUNCTION_LIST                                                                                \
    RUNTIME_FUNCTION_ENTRY(rtSetDevice, int32_t devId)                                                       \
    RUNTIME_FUNCTION_ENTRY(rtSetDeviceEx, int32_t devId)                                                     \
    RUNTIME_FUNCTION_ENTRY(rtSetDeviceWithFlags, int32_t deviceId, uint64_t flags)                           \
    RUNTIME_FUNCTION_ENTRY(rtMalloc, void **devPtr, uint64_t size, rtMemType_t type,                         \
                           const uint16_t moduleId)                                                          \
    RUNTIME_FUNCTION_ENTRY(rtMallocCached, void **devPtr, uint64_t size, rtMemType_t type,                   \
                           const uint16_t moduleId)                                                          \
    RUNTIME_FUNCTION_ENTRY(rtDvppMalloc, void **devPtr, uint64_t size, uint16_t moduleId)                    \
    RUNTIME_FUNCTION_ENTRY(rtDvppMallocWithFlag, void **devPtr, uint64_t size, uint32_t flag,                \
                           const uint16_t moduleId)                                                          \
    RUNTIME_FUNCTION_ENTRY(rtMemAlloc, void **devPtr, uint64_t size, rtMallocPolicy policy,                  \
                           rtMallocAdvise advise, rtMallocConfig_t *cfg)                                     \
    RUNTIME_FUNCTION_ENTRY(rtMemAllocManaged, void **ptr, uint64_t size, uint32_t flag,                      \
                           const uint16_t moduleid)                                                          \
    RUNTIME_FUNCTION_ENTRY(rtMallocPhysical, rtDrvMemHandle *handle, size_t size, rtDrvMemProp_t *prop,      \
                           uint64_t flags)                                                                   \
    RUNTIME_FUNCTION_ENTRY(rtKernelLaunch, const void *stubFunc, uint32_t blockDim, void *args,              \
                           uint32_t argsSize, rtSmDesc_t *smDesc, rtStream_t stm)                            \
    RUNTIME_FUNCTION_ENTRY(rtKernelLaunchWithHandle, void *hdl, const uint64_t tilingKey, uint32_t blockDim, \
                           rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, const void *kernelInfo) \
    RUNTIME_FUNCTION_ENTRY(rtKernelLaunchWithHandleV2, void *hdl, const uint64_t tilingKey,                  \
                           uint32_t blockDim, rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm,      \
                           const rtTaskCfgInfo_t *cfgInfo)                                                   \
    RUNTIME_FUNCTION_ENTRY(rtKernelLaunchWithFlag, const void *stubFunc, uint32_t blockDim,                  \
                           rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags)         \
    RUNTIME_FUNCTION_ENTRY(rtKernelLaunchWithFlagV2, const void *stubFunc, uint32_t blockDim,                \
                           rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags,         \
                           const rtTaskCfgInfo_t *cfgInfo)                                                   \
    RUNTIME_FUNCTION_ENTRY(rtKernelLaunchEx, void *args, uint32_t argsSize, uint32_t flags, rtStream_t stm)  \
    RUNTIME_FUNCTION_ENTRY(rtKernelLaunchFwk, const char_t *opName, void *args, uint32_t argsSize,           \
                           uint32_t flags, rtStream_t rtStream)                                              \
    RUNTIME_FUNCTION_ENTRY(rtCpuKernelLaunch, const void *soName, const void *kernelName, uint32_t blockDim, \
                           const void *args, uint32_t argsSize, rtSmDesc_t *smDesc, rtStream_t stm)          \
    RUNTIME_FUNCTION_ENTRY(rtAicpuKernelLaunch, const rtKernelLaunchNames_t *launchNames, uint32_t blockDim, \
                           const void *args, uint32_t argsSize, rtSmDesc_t *smDesc, rtStream_t stm)          \
    RUNTIME_FUNCTION_ENTRY(rtCpuKernelLaunchWithFlag, const void *soName, const void *kernelName,            \
                           uint32_t blockDim, const rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc,                \
                           rtStream_t stm, uint32_t flags)                                                   \
    RUNTIME_FUNCTION_ENTRY(rtAicpuKernelLaunchWithFlag, const rtKernelLaunchNames_t *launchNames,            \
                           uint32_t blockDim, const rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc,                \
                           rtStream_t stm, uint32_t flags)                                                   \
    RUNTIME_FUNCTION_ENTRY(rtAicpuKernelLaunchExWithArgs, const uint32_t kernelType,                         \
                           const char *const opName, const uint32_t blockDim,                                \
                           const rtAicpuArgsEx_t *argsInfo, rtSmDesc_t *const smDesc, const rtStream_t stm,  \
                           const uint32_t flags)                                                             \
    RUNTIME_FUNCTION_ENTRY(rtLaunchKernelByFuncHandle, rtFuncHandle funcHandle, uint32_t blockDim,           \
                           rtLaunchArgsHandle argsHandle, rtStream_t stm)                                    \
    RUNTIME_FUNCTION_ENTRY(rtLaunchKernelByFuncHandleV2, rtFuncHandle funcHandle, uint32_t blockDim,         \
                           rtLaunchArgsHandle argsHandle, rtStream_t stm, const rtTaskCfgInfo_t *cfgInfo)    \
    RUNTIME_FUNCTION_ENTRY(rtLaunchKernelByFuncHandleV3, rtFuncHandle funcHandle, uint32_t blockDim,         \
                           const rtArgsEx_t *const argsInfo, rtStream_t stm,                                 \
                           const rtTaskCfgInfo_t *const cfgInfo)                                             \
    RUNTIME_FUNCTION_ENTRY(rtVectorCoreKernelLaunchWithHandle, void *hdl, const uint64_t tilingKey,          \
                           uint32_t blockDim, rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm,      \
                           const rtTaskCfgInfo_t *cfgInfo)                                                   \
    RUNTIME_FUNCTION_ENTRY(rtVectorCoreKernelLaunch, const void *stubFunc, uint32_t blockDim,                \
                           rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags,         \
                           const rtTaskCfgInfo_t *cfgInfo)                                                   \
    RUNTIME_FUNCTION_ENTRY(rtFftsPlusTaskLaunch, rtFftsPlusTaskInfo_t *fftsPlusTaskInfo, rtStream_t stm)     \
    RUNTIME_FUNCTION_ENTRY(rtFftsPlusTaskLaunchWithFlag, rtFftsPlusTaskInfo_t *fftsPlusTaskInfo,             \
                           rtStream_t stm, uint32_t flag)                                                    \
    RUNTIME_FUNCTION_ENTRY(rtFftsTaskLaunch, rtFftsTaskInfo_t *fftsTaskInfo, rtStream_t stm)                 \
    RUNTIME_FUNCTION_ENTRY(rtFftsTaskLaunchWithFlag, rtFftsTaskInfo_t *fftsTaskInfo, rtStream_t stm,         \
                           uint32_t flag)                                                                    \
    RUNTIME_FUNCTION_ENTRY(rtModelExecute, rtModel_t mdl, rtStream_t stm, uint32_t flag)                     \
    RUNTIME_FUNCTION_ENTRY(rtModelExecuteAsync, rtModel_t mdl, rtStream_t stm, uint32_t flag)                \
    RUNTIME_FUNCTION_ENTRY(rtStreamBeginCapture, rtStream_t stm, const rtStreamCaptureMode mode)             \
    RUNTIME_FUNCTION_ENTRY(rtStreamEndCapture, rtStream_t stm, rtModel_t *captureMdl)                        \
    RUNTIME_FUNCTION_ENTRY(rtsModelExecute, rtModel_t mdl, int32_t timeout)                                  \
    RUNTIME_FUNCTION_ENTRY(rtModelExecuteSync, rtModel_t mdl, rtStream_t stm, uint32_t flag,                 \
                           int32_t timeout)                                                                  \
    RUNTIME_FUNCTION_ENTRY(rtStarsTaskLaunch, const void *taskSqe, uint32_t sqeLen, rtStream_t stm)          \
    RUNTIME_FUNCTION_ENTRY(rtStarsTaskLaunchWithFlag, const void *taskSqe, uint32_t sqeLen, rtStream_t stm,  \
                           uint32_t flag)                                                                    \
    RUNTIME_FUNCTION_ENTRY(rtCmoTaskLaunch, rtCmoTaskInfo_t *taskInfo, rtStream_t stm, uint32_t flag)        \
    RUNTIME_FUNCTION_ENTRY(rtCmoAddrTaskLaunch, void *cmoAddrInfo, uint64_t destMax,                         \
                           rtCmoOpCode_t cmoOpCode, rtStream_t stm, uint32_t flag)                           \
    RUNTIME_FUNCTION_ENTRY(rtBarrierTaskLaunch, rtBarrierTaskInfo_t *taskInfo, rtStream_t stm,               \
                           uint32_t flag)                                                                    \
    RUNTIME_FUNCTION_ENTRY(rtMultipleTaskInfoLaunch, const void *taskInfo, rtStream_t stm)                   \
    RUNTIME_FUNCTION_ENTRY(rtMultipleTaskInfoLaunchWithFlag, const void *taskInfo, rtStream_t stm,           \
                           const uint32_t flag)                                                              \
    RUNTIME_FUNCTION_ENTRY(rtStreamSynchronize, rtStream_t stm)                                              \
    RUNTIME_FUNCTION_ENTRY(rtEventCreate, rtEvent_t *evt)                                                    \
    RUNTIME_FUNCTION_ENTRY(rtsEventCreate, rtEvent_t *evt, uint64_t flag)                                    \
    RUNTIME_FUNCTION_ENTRY(rtsEventCreateEx, rtEvent_t *evt, uint64_t flag)                                  \
    RUNTIME_FUNCTION_ENTRY(rtEventCreateWithFlag, rtEvent_t *evt, uint32_t flag)                             \
    RUNTIME_FUNCTION_ENTRY(rtEventCreateExWithFlag, rtEvent_t *evt, uint32_t flag)                           \
    RUNTIME_FUNCTION_ENTRY(rtStreamWaitEvent, rtStream_t stm, rtEvent_t evt)                                 \
    RUNTIME_FUNCTION_ENTRY(rtEventRecord, rtEvent_t evt, rtStream_t stm)                                     \
    RUNTIME_FUNCTION_ENTRY(rtEventDestroy, rtEvent_t evt)                                                    \
    RUNTIME_FUNCTION_ENTRY(rtEventReset, rtEvent_t evt, rtStream_t stm)

#define RUNTIME_FUNCTION_ENTRY(name, ...) rtError_t name(__VA_ARGS__);
RUNTIME_FUNCTION_LIST
#undef RUNTIME_FUNCTION_ENTRY

#if defined(__cplusplus)
}
#endif

#endif // RT_H