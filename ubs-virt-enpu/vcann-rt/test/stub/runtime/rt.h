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

#ifndef RT_H
#define RT_H

#include <acl/acl.h>
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
typedef uint32_t rtMemType_t;
typedef void *rtModel_t;
typedef void *rtNotify_t;
typedef void *rtCntNotify_t;
typedef void *rtCntNtyRecordInfo_t;
typedef void *rtCntNtyWaitInfo_t;
typedef void *rtCntNotifyRecordInfo_t;
typedef void *rtCntNotifyWaitInfo_t;
typedef void *rtSmDesc_t;
typedef void *rtStream_t;
typedef void *rtEvent_t;
typedef void *rtTaskCfgInfo_t;
typedef void *rtDrvMemHandle;
typedef void *rtFuncHandle;
typedef void *rtLaunchArgsHandle;
typedef void *rtKernelLaunchCfg_t;
typedef void *rtPlaceHolderInfo_t;
typedef void *rtCpuKernelArgs_t;
typedef void *rtArgsHandle;
typedef void *rtRandomNumTaskInfo_t;
typedef void *rtReduceInfo_t;
typedef void *rtTaskUpdateCfg_t;
typedef void *rtTaskGrp_t;
typedef void *rtTask_t;

typedef struct rtDim3 {
    uint32_t x;
    uint32_t y;
    uint32_t z;
} rtDim3;

typedef struct dim3 {
    uint32_t x;
    uint32_t y;
    uint32_t z;
} dim3;
#define RT_STUB_RT_DIM3_DEFINED

typedef void *aclrtFuncHandle;
typedef void *aclmdlRI;
typedef void *aclrtArgsHandle;
typedef void *aclrtDrvMemHandle;
typedef int aclrtMemMallocPolicy;
typedef int aclmdlRICaptureMode;
typedef int aclrtReduceKind;
typedef int aclDataType;
typedef int aclrtMemAttr;
typedef struct aclrtRandomNumTaskInfo aclrtRandomNumTaskInfo;
typedef struct aclrtTaskUpdateInfo aclrtTaskUpdateInfo;
typedef struct aclrtPhysicalMemProp aclrtPhysicalMemProp;

typedef struct aclrtLaunchKernelCfg {
    int reserved;
} aclrtLaunchKernelCfg;

typedef struct aclrtPlaceHolderInfo {
    int reserved;
} aclrtPlaceHolderInfo;

typedef struct aclrtCntNotifyRecordInfo {
    int reserved;
} aclrtCntNotifyRecordInfo;

typedef struct aclrtCntNotifyWaitInfo {
    int reserved;
} aclrtCntNotifyWaitInfo;

typedef enum tagRtStreamCaptureMode
{
    RT_STREAM_CAPTURE_MODE_GLOBAL = 0,
    RT_STREAM_CAPTURE_MODE_THREAD_LOCAL = 1,
    RT_STREAM_CAPTURE_MODE_RELAXED = 2,
    RT_STREAM_CAPTURE_MODE_MAX
} rtStreamCaptureMode;

typedef enum tagRtStreamCaptureStatus
{
    RT_STREAM_CAPTURE_STATUS_NONE = 0,
    RT_STREAM_CAPTURE_STATUS_ACTIVE = 1,
    RT_STREAM_CAPTURE_STATUS_INVALIDATED = 2,
    RT_STREAM_CAPTURE_STATUS_MAX
} rtStreamCaptureStatus;

typedef enum
{
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

typedef enum
{
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

typedef enum
{
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

typedef enum tagRtMemInfoType
{
    RT_MEMORYINFO_DDR,
    RT_MEMORYINFO_HBM,
    RT_MEMORYINFO_DDR_HUGE,       // Hugepage memory of DDR
    RT_MEMORYINFO_DDR_NORMAL,     // Normal memory of DDR
    RT_MEMORYINFO_HBM_HUGE,       // Hugepage memory of HBM
    RT_MEMORYINFO_HBM_NORMAL,     // Normal memory of HBM
    RT_MEMORYINFO_DDR_P2P_HUGE,   // Hugepage memory of DDR
    RT_MEMORYINFO_DDR_P2P_NORMAL, // Normal memory of DDR
    RT_MEMORYINFO_HBM_P2P_HUGE,   // Hugepage memory of HBM
    RT_MEMORYINFO_HBM_P2P_NORMAL, // Normal memory of HBM
    RT_MEMORYINFO_HBM_HUGE1G,     // 1G HugePage memory of HBM
    RT_MEMORYINFO_HBM_P2P_HUGE1G, // 1G HugePage memory of HBM
} rtMemInfoType_t;

#define RUNTIME_FUNCTION_LIST                                                                                          \
    RUNTIME_FUNCTION_ENTRY(rtSetDevice, int32_t devId)                                                                 \
    RUNTIME_FUNCTION_ENTRY(rtSetDeviceEx, int32_t devId)                                                               \
    RUNTIME_FUNCTION_ENTRY(rtSetDeviceWithFlags, int32_t deviceId, uint64_t flags)                                     \
    RUNTIME_FUNCTION_ENTRY(rtMalloc, void **devPtr, uint64_t size, rtMemType_t type, const uint16_t moduleId)          \
    RUNTIME_FUNCTION_ENTRY(rtSetDeviceWithoutTsd, int32_t deviceId)                                                    \
    RUNTIME_FUNCTION_ENTRY(rtGetSocVersion, char_t *ver, const uint32_t maxLen)                                        \
    RUNTIME_FUNCTION_ENTRY(rtMallocCached, void **devPtr, uint64_t size, rtMemType_t type, const uint16_t moduleId)    \
    RUNTIME_FUNCTION_ENTRY(rtDvppMalloc, void **devPtr, uint64_t size, uint16_t moduleId)                              \
    RUNTIME_FUNCTION_ENTRY(rtDvppMallocWithFlag, void **devPtr, uint64_t size, uint32_t flag, const uint16_t moduleId) \
    RUNTIME_FUNCTION_ENTRY(rtMemAlloc, void **devPtr, uint64_t size, rtMallocPolicy policy, rtMallocAdvise advise,     \
                           rtMallocConfig_t *cfg)                                                                      \
    RUNTIME_FUNCTION_ENTRY(rtMemAllocManaged, void **ptr, uint64_t size, uint32_t flag, const uint16_t moduleid)       \
    RUNTIME_FUNCTION_ENTRY(rtMallocPhysical, rtDrvMemHandle *handle, size_t size, rtDrvMemProp_t *prop,                \
                           uint64_t flags)                                                                             \
    RUNTIME_FUNCTION_ENTRY(rtMemGetInfoEx, rtMemInfoType_t memInfoType, size_t *freeSize, size_t *totalSize)           \
    RUNTIME_FUNCTION_ENTRY(rtKernelLaunch, const void *stubFunc, uint32_t blockDim, void *args, uint32_t argsSize,     \
                           rtSmDesc_t *smDesc, rtStream_t stm)                                                         \
    RUNTIME_FUNCTION_ENTRY(rtKernelLaunchWithHandle, void *hdl, const uint64_t tilingKey, uint32_t blockDim,           \
                           rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, const void *kernelInfo)           \
    RUNTIME_FUNCTION_ENTRY(rtKernelLaunchWithHandleV2, void *hdl, const uint64_t tilingKey, uint32_t blockDim,         \
                           rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, const rtTaskCfgInfo_t *cfgInfo)   \
    RUNTIME_FUNCTION_ENTRY(rtKernelLaunchWithFlag, const void *stubFunc, uint32_t blockDim, rtArgsEx_t *argsInfo,      \
                           rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags)                                         \
    RUNTIME_FUNCTION_ENTRY(rtKernelLaunchWithFlagV2, const void *stubFunc, uint32_t blockDim, rtArgsEx_t *argsInfo,    \
                           rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags, const rtTaskCfgInfo_t *cfgInfo)         \
    RUNTIME_FUNCTION_ENTRY(rtKernelLaunchEx, void *args, uint32_t argsSize, uint32_t flags, rtStream_t stm)            \
    RUNTIME_FUNCTION_ENTRY(rtKernelLaunchFwk, const char_t *opName, void *args, uint32_t argsSize, uint32_t flags,     \
                           rtStream_t rtStream)                                                                        \
    RUNTIME_FUNCTION_ENTRY(rtCpuKernelLaunchWithFlag, const void *soName, const void *kernelName, uint32_t blockDim,   \
                           const rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags)             \
    RUNTIME_FUNCTION_ENTRY(rtAicpuKernelLaunchWithFlag, const rtKernelLaunchNames_t *launchNames, uint32_t blockDim,   \
                           const rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags)             \
    RUNTIME_FUNCTION_ENTRY(rtAicpuKernelLaunchExWithArgs, const uint32_t kernelType, const char *const opName,         \
                           const uint32_t blockDim, const rtAicpuArgsEx_t *argsInfo, rtSmDesc_t *const smDesc,         \
                           const rtStream_t stm, const uint32_t flags)                                                 \
    RUNTIME_FUNCTION_ENTRY(rtLaunchKernelByFuncHandle, rtFuncHandle funcHandle, uint32_t blockDim,                     \
                           rtLaunchArgsHandle argsHandle, rtStream_t stm)                                              \
    RUNTIME_FUNCTION_ENTRY(rtLaunchKernelByFuncHandleV2, rtFuncHandle funcHandle, uint32_t blockDim,                   \
                           rtLaunchArgsHandle argsHandle, rtStream_t stm, const rtTaskCfgInfo_t *cfgInfo)              \
    RUNTIME_FUNCTION_ENTRY(rtLaunchKernelByFuncHandleV3, rtFuncHandle funcHandle, uint32_t blockDim,                   \
                           const rtArgsEx_t *const argsInfo, rtStream_t stm, const rtTaskCfgInfo_t *const cfgInfo)     \
    RUNTIME_FUNCTION_ENTRY(rtVectorCoreKernelLaunchWithHandle, void *hdl, const uint64_t tilingKey, uint32_t blockDim, \
                           rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, const rtTaskCfgInfo_t *cfgInfo)   \
    RUNTIME_FUNCTION_ENTRY(rtVectorCoreKernelLaunch, const void *stubFunc, uint32_t blockDim, rtArgsEx_t *argsInfo,    \
                           rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags, const rtTaskCfgInfo_t *cfgInfo)         \
    RUNTIME_FUNCTION_ENTRY(rtFftsPlusTaskLaunch, rtFftsPlusTaskInfo_t *fftsPlusTaskInfo, rtStream_t stm)               \
    RUNTIME_FUNCTION_ENTRY(rtFftsPlusTaskLaunchWithFlag, rtFftsPlusTaskInfo_t *fftsPlusTaskInfo, rtStream_t stm,       \
                           uint32_t flag)                                                                              \
    RUNTIME_FUNCTION_ENTRY(rtModelExecute, rtModel_t mdl, rtStream_t stm, uint32_t flag)                               \
    RUNTIME_FUNCTION_ENTRY(rtModelExecuteAsync, rtModel_t mdl, rtStream_t stm, uint32_t flag)                          \
    RUNTIME_FUNCTION_ENTRY(rtStreamBeginCapture, rtStream_t stm, const rtStreamCaptureMode mode)                       \
    RUNTIME_FUNCTION_ENTRY(rtStreamEndCapture, rtStream_t stm, rtModel_t *captureMdl)                                  \
    RUNTIME_FUNCTION_ENTRY(rtModelGetStreams, rtModel_t const mdl, rtStream_t *streams, uint32_t *numStreams)          \
    RUNTIME_FUNCTION_ENTRY(rtStreamGetTasks, rtStream_t const stm, rtTask_t *tasks, uint32_t *numTasks)                \
    RUNTIME_FUNCTION_ENTRY(rtStreamGetCaptureInfo, rtStream_t stm, rtStreamCaptureStatus *const status,                \
                           rtModel_t *captureMdl)                                                                      \
    RUNTIME_FUNCTION_ENTRY(rtEventElapsedTime, float *timeInterval, rtEvent_t startEvent, rtEvent_t endEvent)          \
    RUNTIME_FUNCTION_ENTRY(rtsModelExecute, rtModel_t mdl, int32_t timeout)                                            \
    RUNTIME_FUNCTION_ENTRY(rtModelExecuteSync, rtModel_t mdl, rtStream_t stm, uint32_t flag, int32_t timeout)          \
    RUNTIME_FUNCTION_ENTRY(rtsModelExecuteAsync, rtModel_t mdl, rtStream_t stm)                                        \
    RUNTIME_FUNCTION_ENTRY(rtStarsTaskLaunch, const void *taskSqe, uint32_t sqeLen, rtStream_t stm)                    \
    RUNTIME_FUNCTION_ENTRY(rtStarsTaskLaunchWithFlag, const void *taskSqe, uint32_t sqeLen, rtStream_t stm,            \
                           uint32_t flag)                                                                              \
    RUNTIME_FUNCTION_ENTRY(rtMultipleTaskInfoLaunch, const void *taskInfo, rtStream_t stm)                             \
    RUNTIME_FUNCTION_ENTRY(rtStreamSynchronize, rtStream_t stm)                                                        \
    RUNTIME_FUNCTION_ENTRY(rtEventCreate, rtEvent_t *evt)                                                              \
    RUNTIME_FUNCTION_ENTRY(rtsEventCreate, rtEvent_t *evt, uint64_t flag)                                              \
    RUNTIME_FUNCTION_ENTRY(rtsEventCreateEx, rtEvent_t *evt, uint64_t flag)                                            \
    RUNTIME_FUNCTION_ENTRY(rtEventCreateWithFlag, rtEvent_t *evt, uint32_t flag)                                       \
    RUNTIME_FUNCTION_ENTRY(rtEventCreateExWithFlag, rtEvent_t *evt, uint32_t flag)                                     \
    RUNTIME_FUNCTION_ENTRY(rtStreamWaitEvent, rtStream_t stm, rtEvent_t evt)                                           \
    RUNTIME_FUNCTION_ENTRY(rtEventRecord, rtEvent_t evt, rtStream_t stm)                                               \
    RUNTIME_FUNCTION_ENTRY(rtEventDestroy, rtEvent_t evt)                                                              \
    RUNTIME_FUNCTION_ENTRY(rtEventDestroySync, rtEvent_t evt)                                                          \
    RUNTIME_FUNCTION_ENTRY(rtsNotifyCreate, rtNotify_t *notify, uint64_t flag)                                         \
    RUNTIME_FUNCTION_ENTRY(rtNotifyRecord, rtNotify_t notify, rtStream_t stm)                                          \
    RUNTIME_FUNCTION_ENTRY(rtNotifyDestroy, rtNotify_t notify)                                                         \
    RUNTIME_FUNCTION_ENTRY(rtStreamDestroy, rtStream_t stm)                                                            \
    RUNTIME_FUNCTION_ENTRY(rtsNotifyWaitAndReset, rtNotify_t notify, rtStream_t stm, uint32_t timeout)                 \
    RUNTIME_FUNCTION_ENTRY(rtStreamWaitEventWithTimeout, rtStream_t stm, rtEvent_t evt, uint32_t timeout)              \
    RUNTIME_FUNCTION_ENTRY(rtNotifyWait, rtNotify_t notify, rtStream_t stm)                                            \
    RUNTIME_FUNCTION_ENTRY(rtNotifyWaitWithTimeOut, rtNotify_t notify, rtStream_t stm, uint32_t timeOut)               \
    RUNTIME_FUNCTION_ENTRY(rtNotifyCreateWithFlag, int32_t deviceId, rtNotify_t *notify, uint32_t flag)                \
    RUNTIME_FUNCTION_ENTRY(rtCntNotifyCreate, const int32_t deviceId, rtCntNotify_t *const cntNotify)                  \
    RUNTIME_FUNCTION_ENTRY(rtCntNotifyCreateWithFlag, const int32_t deviceId, rtCntNotify_t *const cntNotify,          \
                           const uint32_t flags)                                                                       \
    RUNTIME_FUNCTION_ENTRY(rtCntNotifyRecord, rtCntNotify_t const inCntNotify, rtStream_t const stm,                   \
                           const rtCntNtyRecordInfo_t *const info)                                                     \
    RUNTIME_FUNCTION_ENTRY(rtCntNotifyWaitWithTimeout, rtCntNotify_t const inCntNotify, rtStream_t const stm,          \
                           const rtCntNtyWaitInfo_t *const info)                                                       \
    RUNTIME_FUNCTION_ENTRY(rtCntNotifyDestroy, rtCntNotify_t const inCntNotify)                                        \
    RUNTIME_FUNCTION_ENTRY(rtsCntNotifyRecord, rtCntNotify_t cntNotify, rtStream_t stm, rtCntNotifyRecordInfo_t *info) \
    RUNTIME_FUNCTION_ENTRY(rtsCntNotifyWaitWithTimeout, rtCntNotify_t cntNotify, rtStream_t stm,                       \
                           rtCntNotifyWaitInfo_t *info)                                                                \
    RUNTIME_FUNCTION_ENTRY(rtsLaunchKernelWithHostArgs, rtFuncHandle funcHandle, uint32_t numBlocks, rtStream_t stm,   \
                           rtKernelLaunchCfg_t *cfg, void *hostArgs, uint32_t argsSize,                                \
                           rtPlaceHolderInfo_t *placeHolderArray, uint32_t placeHolderNum)                             \
    RUNTIME_FUNCTION_ENTRY(rtsLaunchKernelWithConfig, rtFuncHandle funcHandle, uint32_t numBlocks, rtStream_t stm,     \
                           rtKernelLaunchCfg_t *cfg, rtArgsHandle argsHandle, void *reserve)                           \
    RUNTIME_FUNCTION_ENTRY(rtsLaunchKernelWithDevArgs, rtFuncHandle funcHandle, uint32_t numBlocks, rtStream_t stm,    \
                           rtKernelLaunchCfg_t *cfg, const void *args, uint32_t argsSize, void *reserve)               \
    RUNTIME_FUNCTION_ENTRY(rtsLaunchRandomNumTask, const rtRandomNumTaskInfo_t *taskInfo, const rtStream_t stm,        \
                           void *reserve)                                                                              \
    RUNTIME_FUNCTION_ENTRY(rtsLaunchReduceAsyncTask, const rtReduceInfo_t *reduceInfo, const rtStream_t stm,           \
                           const void *reserve)                                                                        \
    RUNTIME_FUNCTION_ENTRY(rtsLaunchUpdateTask, rtStream_t destStm, uint32_t destTaskId, rtStream_t stm,               \
                           rtTaskUpdateCfg_t *cfg)                                                                     \
    RUNTIME_FUNCTION_ENTRY(aclrtSetDeviceImpl, int32_t devId)                                                          \
    RUNTIME_FUNCTION_ENTRY(aclrtSetDeviceWithoutTsdVXXImpl, int32_t devId)                                             \
    RUNTIME_FUNCTION_ENTRY(aclrtMallocImpl, void **devPtr, size_t size, aclrtMemMallocPolicy policy)                   \
    RUNTIME_FUNCTION_ENTRY(aclrtMallocAlign32Impl, void **devPtr, size_t size, aclrtMemMallocPolicy policy)            \
    RUNTIME_FUNCTION_ENTRY(aclrtMallocCachedImpl, void **devPtr, size_t size, aclrtMemMallocPolicy policy)             \
    RUNTIME_FUNCTION_ENTRY(aclrtMemAllocManagedImpl, void **ptr, uint64_t size, uint32_t flag)                         \
    RUNTIME_FUNCTION_ENTRY(aclrtMallocPhysicalImpl, aclrtDrvMemHandle *handle, size_t size,                            \
                           const aclrtPhysicalMemProp *prop, uint64_t flags)                                           \
    RUNTIME_FUNCTION_ENTRY(aclrtGetMemInfoImpl, aclrtMemAttr attr, size_t *free, size_t *total)                        \
    RUNTIME_FUNCTION_ENTRY(aclmdlRIExecuteAsyncImpl, aclmdlRI modelRI, aclrtStream stream)                             \
    RUNTIME_FUNCTION_ENTRY(aclmdlRIExecuteImpl, aclmdlRI modelRI, int32_t timeout)                                     \
    RUNTIME_FUNCTION_ENTRY(aclmdlRICaptureBeginImpl, aclrtStream stream, aclmdlRICaptureMode mode)                     \
    RUNTIME_FUNCTION_ENTRY(aclmdlRICaptureEndImpl, aclrtStream stream, aclmdlRI *modelRI)                              \
    RUNTIME_FUNCTION_ENTRY(aclrtLaunchKernelImpl, aclrtFuncHandle funcHandle, uint32_t numBlocks,                      \
                           const void *argsData, size_t argsSize, aclrtStream stream)                                  \
    RUNTIME_FUNCTION_ENTRY(aclrtLaunchKernelWithHostArgsImpl, aclrtFuncHandle funcHandle, uint32_t numBlocks,          \
                           aclrtStream stream, aclrtLaunchKernelCfg *cfg, void *hostArgs, size_t argsSize,             \
                           aclrtPlaceHolderInfo *placeHolderArray, size_t placeHolderNum)                              \
    RUNTIME_FUNCTION_ENTRY(aclrtLaunchKernelWithConfigImpl, aclrtFuncHandle funcHandle, uint32_t numBlocks,            \
                           aclrtStream stream, aclrtLaunchKernelCfg *cfg, aclrtArgsHandle argsHandle, void *reserve)   \
    RUNTIME_FUNCTION_ENTRY(aclrtLaunchKernelV2Impl, aclrtFuncHandle funcHandle, uint32_t numBlocks,                    \
                           const void *argsData, size_t argsSize, aclrtLaunchKernelCfg *cfg, aclrtStream stream)       \
    RUNTIME_FUNCTION_ENTRY(aclrtRandomNumAsyncImpl, const aclrtRandomNumTaskInfo *taskInfo, const aclrtStream stream,  \
                           void *reserve)                                                                              \
    RUNTIME_FUNCTION_ENTRY(aclrtReduceAsyncImpl, void *dst, const void *src, uint64_t count, aclrtReduceKind kind,     \
                           aclDataType type, aclrtStream stream, void *reserve)                                        \
    RUNTIME_FUNCTION_ENTRY(aclrtTaskUpdateAsyncImpl, aclrtStream taskStream, uint32_t taskId,                          \
                           aclrtTaskUpdateInfo *info, aclrtStream execStream)                                          \
    RUNTIME_FUNCTION_ENTRY(rtLaunchKernelWithArgsArray, void *func, uint32_t numBlocks, rtStream_t stm,                \
                           rtKernelLaunchCfg_t *cfg, void **args)                                                      \
    RUNTIME_FUNCTION_ENTRY(aclrtLaunchKernelWithArgsArrayImpl, void *func, uint32_t numBlocks, aclrtStream stream,     \
                           aclrtLaunchKernelCfg *cfg, void **args)                                                     \
    RUNTIME_FUNCTION_ENTRY(rtLaunchSIMTKernelWithHostArgs, void *func, rtDim3 gridDim, rtDim3 blockDim,                \
                           size_t dynUbufSize, rtStream_t stm, rtKernelLaunchCfg_t *cfg, void *hostArgs,               \
                           uint32_t argsSize, rtPlaceHolderInfo_t *placeHolderArray, uint32_t placeHolderNum)          \
    RUNTIME_FUNCTION_ENTRY(aclrtLaunchSIMTKernelWithHostArgsImpl, void *func, dim3 gridDim, dim3 blockDim,             \
                           size_t dynUbufSize, aclrtStream stream, aclrtLaunchKernelCfg *cfg, void *hostArgs,          \
                           size_t argsSize, aclrtPlaceHolderInfo *placeHolderArray, size_t placeHolderNum)             \
    RUNTIME_FUNCTION_ENTRY(rtLaunchSIMTKernelWithArgsArray, void *func, rtDim3 gridDim, rtDim3 blockDim,               \
                           size_t dynUbufSize, rtStream_t stm, rtKernelLaunchCfg_t *cfg, void **args)                  \
    RUNTIME_FUNCTION_ENTRY(aclrtLaunchSIMTKernelWithArgsArrayImpl, void *func, dim3 gridDim, dim3 blockDim,            \
                           size_t dynUbufSize, aclrtStream stream, aclrtLaunchKernelCfg *cfg, void **args)             \
    RUNTIME_FUNCTION_ENTRY(aclrtCreateEventImpl, aclrtEvent *event)                                                    \
    RUNTIME_FUNCTION_ENTRY(aclrtCreateEventWithFlagImpl, aclrtEvent *event, uint32_t flag)                             \
    RUNTIME_FUNCTION_ENTRY(aclrtCreateEventExWithFlagImpl, aclrtEvent *event, uint32_t flag)                           \
    RUNTIME_FUNCTION_ENTRY(aclrtStreamWaitEventImpl, aclrtStream stream, aclrtEvent event)                             \
    RUNTIME_FUNCTION_ENTRY(aclrtRecordEventImpl, aclrtEvent event, aclrtStream stream)                                 \
    RUNTIME_FUNCTION_ENTRY(aclrtRecordEventWithFlagImpl, aclrtEvent event, aclrtStream stream, uint32_t flag)          \
    RUNTIME_FUNCTION_ENTRY(aclrtDestroyEventImpl, aclrtEvent event)                                                    \
    RUNTIME_FUNCTION_ENTRY(aclrtDestroyStream, aclrtStream stream)                                                     \
    RUNTIME_FUNCTION_ENTRY(aclrtCreateNotifyImpl, aclrtNotify *notify, uint64_t flag)                                  \
    RUNTIME_FUNCTION_ENTRY(aclrtWaitAndResetNotifyImpl, aclrtNotify notify, aclrtStream stream, uint32_t timeout)      \
    RUNTIME_FUNCTION_ENTRY(rtNotifyCreate, int32_t deviceId, rtNotify_t *notify)                                       \
    RUNTIME_FUNCTION_ENTRY(aclrtCntNotifyCreateImpl, aclrtCntNotify *cntNotify, uint64_t flag)                         \
    RUNTIME_FUNCTION_ENTRY(aclrtCntNotifyDestroyImpl, aclrtCntNotify cntNotify)                                        \
    RUNTIME_FUNCTION_ENTRY(aclrtCntNotifyRecordImpl, aclrtCntNotify cntNotify, aclrtStream stream,                     \
                           aclrtCntNotifyRecordInfo *info)                                                             \
    RUNTIME_FUNCTION_ENTRY(aclrtCntNotifyWaitWithTimeoutImpl, aclrtCntNotify cntNotify, aclrtStream stream,            \
                           aclrtCntNotifyWaitInfo *info)                                                               \
    RUNTIME_FUNCTION_ENTRY(rtsStreamBeginTaskGrp, rtStream_t stm)                                                      \
    RUNTIME_FUNCTION_ENTRY(aclmdlRICaptureTaskGrpBeginImpl, aclrtStream stream)                                        \
    RUNTIME_FUNCTION_ENTRY(rtsStreamEndTaskGrp, rtStream_t stm, rtTaskGrp_t *handle)                                   \
    RUNTIME_FUNCTION_ENTRY(aclmdlRICaptureTaskGrpEndImpl, aclrtStream stream, aclrtTaskGrp *handle)                    \
    RUNTIME_FUNCTION_ENTRY(rtsStreamBeginTaskUpdate, rtStream_t stm, rtTaskGrp_t handle)                               \
    RUNTIME_FUNCTION_ENTRY(aclmdlRICaptureTaskUpdateBeginImpl, aclrtStream stream, aclrtTaskGrp handle)                \
    RUNTIME_FUNCTION_ENTRY(rtsStreamEndTaskUpdate, rtStream_t stm)                                                     \
    RUNTIME_FUNCTION_ENTRY(aclmdlRICaptureTaskUpdateEndImpl, aclrtStream stream)

#define RUNTIME_FUNCTION_ENTRY(name, ...) rtError_t name(__VA_ARGS__);
RUNTIME_FUNCTION_LIST
#undef RUNTIME_FUNCTION_ENTRY

#if defined(__cplusplus)
}
#endif

#endif // RT_H