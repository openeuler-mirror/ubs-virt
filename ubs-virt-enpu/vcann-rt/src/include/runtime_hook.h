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

#ifndef __RUNTIME_HOOK_H__
#define __RUNTIME_HOOK_H__

#include <acl/acl.h>
#include <dlfcn.h>
#include <runtime/rt.h>
#include <stdarg.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
    void *func_ptr;
    char *name;
} rt_entry_t;

typedef rtError_t (*rt_symbol_t)();

#define RUNTIME_HOOK_ENUM(x) HOOK_##x

#define RUNTIME_HOOK_DEFINE(func_name, ...) __attribute__((visibility("default"))) rtError_t func_name(__VA_ARGS__)

#define RUNTIME_FIND_ENTRY(table, symbol) ({ (table)[RUNTIME_HOOK_ENUM(symbol)].func_ptr; })

#define RUNTIME_HOOK_CALL(table, symbol, ...)                                \
    ({                                                                       \
        rt_symbol_t _entry = (rt_symbol_t)RUNTIME_FIND_ENTRY(table, symbol); \
        if (!_entry) {                                                       \
            fprintf(stderr, "HOOK ERROR: %s - %s\n", #symbol, dlerror());    \
        }                                                                    \
        _entry ? _entry(__VA_ARGS__) : ACL_ERROR_FAILURE;                    \
    })

typedef enum
{
    /* aclrt及对应rt接口，cann-9.2.0，共73个 */
    RUNTIME_HOOK_ENUM(rtSetDevice),
    RUNTIME_HOOK_ENUM(aclrtSetDeviceImpl),
    RUNTIME_HOOK_ENUM(rtSetDeviceWithoutTsd),
    RUNTIME_HOOK_ENUM(aclrtSetDeviceWithoutTsdVXXImpl),
    RUNTIME_HOOK_ENUM(rtGetSocVersion),
    RUNTIME_HOOK_ENUM(rtMalloc),
    RUNTIME_HOOK_ENUM(aclrtMallocImpl),
    RUNTIME_HOOK_ENUM(aclrtMallocAlign32Impl),
    RUNTIME_HOOK_ENUM(rtMallocCached),
    RUNTIME_HOOK_ENUM(aclrtMallocCachedImpl),
    RUNTIME_HOOK_ENUM(rtMemAllocManaged),
    RUNTIME_HOOK_ENUM(aclrtMemAllocManagedImpl),
    RUNTIME_HOOK_ENUM(rtMallocPhysical),
    RUNTIME_HOOK_ENUM(rtFreePhysical),
    RUNTIME_HOOK_ENUM(aclrtMallocPhysicalImpl),
    RUNTIME_HOOK_ENUM(rtMemGetInfoEx),
    RUNTIME_HOOK_ENUM(rtMallocHost),
    RUNTIME_HOOK_ENUM(rtFreeHost),
    RUNTIME_HOOK_ENUM(rtReserveMemAddress),
    RUNTIME_HOOK_ENUM(rtReleaseMemAddress),
    RUNTIME_HOOK_ENUM(rtMapMem),
    RUNTIME_HOOK_ENUM(rtUnmapMem),
    RUNTIME_HOOK_ENUM(rtMemcpy),
    RUNTIME_HOOK_ENUM(rtsPointerGetAttributes),
    RUNTIME_HOOK_ENUM(rtMemcpyAsync),
    RUNTIME_HOOK_ENUM(rtMemGetAllocationGranularity),
    RUNTIME_HOOK_ENUM(rtFree),
    RUNTIME_HOOK_ENUM(rtsCheckMemType),
    RUNTIME_HOOK_ENUM(rtMemcpyAsyncEx),
    RUNTIME_HOOK_ENUM(rtsMemcpyBatch),
    RUNTIME_HOOK_ENUM(rtsMemcpyBatchAsync),
    RUNTIME_HOOK_ENUM(rtMemcpy2d),
    RUNTIME_HOOK_ENUM(rtMemcpy2dAsync),
    RUNTIME_HOOK_ENUM(rtsSetMemcpyDesc),
    RUNTIME_HOOK_ENUM(rtsMemcpyAsyncWithDesc),
    RUNTIME_HOOK_ENUM(rtMemcpyAsyncWithOffset),
    RUNTIME_HOOK_ENUM(rtMemset),
    RUNTIME_HOOK_ENUM(rtMemsetAsync),
    RUNTIME_HOOK_ENUM(rtMemPrefetchToDevice),
    RUNTIME_HOOK_ENUM(rtsIpcMemGetExportKey),
    RUNTIME_HOOK_ENUM(rtsIpcMemImportByKey),
    RUNTIME_HOOK_ENUM(rtsValueWrite),
    RUNTIME_HOOK_ENUM(rtsValueWait),
    RUNTIME_HOOK_ENUM(aclrtGetMemInfoImpl),
    RUNTIME_HOOK_ENUM(rtLaunchKernelByFuncHandleV3),
    RUNTIME_HOOK_ENUM(aclrtLaunchKernelImpl),
    RUNTIME_HOOK_ENUM(rtsLaunchKernelWithHostArgs),
    RUNTIME_HOOK_ENUM(aclrtLaunchKernelWithHostArgsImpl),
    RUNTIME_HOOK_ENUM(rtsLaunchKernelWithConfig),
    RUNTIME_HOOK_ENUM(aclrtLaunchKernelWithConfigImpl),
    RUNTIME_HOOK_ENUM(rtsLaunchKernelWithDevArgs),
    RUNTIME_HOOK_ENUM(aclrtLaunchKernelV2Impl),
    RUNTIME_HOOK_ENUM(rtsLaunchRandomNumTask),
    RUNTIME_HOOK_ENUM(aclrtRandomNumAsyncImpl),
    RUNTIME_HOOK_ENUM(rtsLaunchReduceAsyncTask),
    RUNTIME_HOOK_ENUM(aclrtReduceAsyncImpl),
    RUNTIME_HOOK_ENUM(rtsLaunchUpdateTask),
    RUNTIME_HOOK_ENUM(aclrtTaskUpdateAsyncImpl),
    RUNTIME_HOOK_ENUM(rtLaunchKernelWithArgsArray),
    RUNTIME_HOOK_ENUM(aclrtLaunchKernelWithArgsArrayImpl),
    RUNTIME_HOOK_ENUM(rtLaunchSIMTKernelWithHostArgs),
    RUNTIME_HOOK_ENUM(aclrtLaunchSIMTKernelWithHostArgsImpl),
    RUNTIME_HOOK_ENUM(rtLaunchSIMTKernelWithArgsArray),
    RUNTIME_HOOK_ENUM(aclrtLaunchSIMTKernelWithArgsArrayImpl),
    RUNTIME_HOOK_ENUM(rtModelExecute),
    RUNTIME_HOOK_ENUM(aclmdlRIExecuteAsyncImpl),
    RUNTIME_HOOK_ENUM(rtStreamBeginCapture),
    RUNTIME_HOOK_ENUM(aclmdlRICaptureBeginImpl),
    RUNTIME_HOOK_ENUM(rtStreamEndCapture),
    RUNTIME_HOOK_ENUM(aclmdlRICaptureEndImpl),
    RUNTIME_HOOK_ENUM(rtsModelExecute),
    RUNTIME_HOOK_ENUM(aclmdlRIExecuteImpl),
    RUNTIME_HOOK_ENUM(rtsStreamBeginTaskGrp),
    RUNTIME_HOOK_ENUM(aclmdlRICaptureTaskGrpBeginImpl),
    RUNTIME_HOOK_ENUM(rtsStreamEndTaskGrp),
    RUNTIME_HOOK_ENUM(aclmdlRICaptureTaskGrpEndImpl),
    RUNTIME_HOOK_ENUM(rtsStreamBeginTaskUpdate),
    RUNTIME_HOOK_ENUM(aclmdlRICaptureTaskUpdateBeginImpl),
    RUNTIME_HOOK_ENUM(rtsStreamEndTaskUpdate),
    RUNTIME_HOOK_ENUM(aclmdlRICaptureTaskUpdateEndImpl),
    RUNTIME_HOOK_ENUM(rtStreamGetCaptureInfo),
    RUNTIME_HOOK_ENUM(rtModelGetStreams),
    RUNTIME_HOOK_ENUM(rtStreamGetTasks),
    RUNTIME_HOOK_ENUM(rtEventCreate),
    RUNTIME_HOOK_ENUM(aclrtCreateEventImpl),
    RUNTIME_HOOK_ENUM(rtEventCreateWithFlag),
    RUNTIME_HOOK_ENUM(aclrtCreateEventWithFlagImpl),
    RUNTIME_HOOK_ENUM(rtEventCreateExWithFlag),
    RUNTIME_HOOK_ENUM(aclrtCreateEventExWithFlagImpl),
    RUNTIME_HOOK_ENUM(rtStreamWaitEvent),
    RUNTIME_HOOK_ENUM(aclrtStreamWaitEventImpl),
    RUNTIME_HOOK_ENUM(rtEventRecord),
    RUNTIME_HOOK_ENUM(aclrtRecordEventImpl),
    RUNTIME_HOOK_ENUM(aclrtRecordEventWithFlagImpl),
    RUNTIME_HOOK_ENUM(rtEventElapsedTime),
    RUNTIME_HOOK_ENUM(rtEventDestroy),
    RUNTIME_HOOK_ENUM(aclrtDestroyEventImpl),
    RUNTIME_HOOK_ENUM(rtsNotifyCreate),
    RUNTIME_HOOK_ENUM(aclrtCreateNotifyImpl),
    RUNTIME_HOOK_ENUM(rtsNotifyWaitAndReset),
    RUNTIME_HOOK_ENUM(aclrtWaitAndResetNotifyImpl),
    RUNTIME_HOOK_ENUM(rtCntNotifyCreate),
    RUNTIME_HOOK_ENUM(rtCntNotifyCreateWithFlag),
    RUNTIME_HOOK_ENUM(aclrtCntNotifyCreateImpl),
    RUNTIME_HOOK_ENUM(rtCntNotifyDestroy),
    RUNTIME_HOOK_ENUM(aclrtCntNotifyDestroyImpl),
    RUNTIME_HOOK_ENUM(rtsCntNotifyRecord),
    RUNTIME_HOOK_ENUM(aclrtCntNotifyRecordImpl),
    RUNTIME_HOOK_ENUM(rtsCntNotifyWaitWithTimeout),
    RUNTIME_HOOK_ENUM(aclrtCntNotifyWaitWithTimeoutImpl),
    RUNTIME_HOOK_ENUM(rtStreamSynchronize),
    RUNTIME_HOOK_ENUM(rtStreamDestroy),
    RUNTIME_HOOK_ENUM(aclrtDestroyStream),
    RUNTIME_HOOK_ENUM(rtSetDeviceWithFlags),
    RUNTIME_HOOK_ENUM(rtDvppMallocWithFlag),
    RUNTIME_HOOK_ENUM(rtMemAlloc),
    RUNTIME_HOOK_ENUM(rtCpuKernelLaunchWithFlag),
    RUNTIME_HOOK_ENUM(rtAicpuKernelLaunchExWithArgs),
    RUNTIME_HOOK_ENUM(rtFftsPlusTaskLaunch),
    RUNTIME_HOOK_ENUM(rtFftsPlusTaskLaunchWithFlag),
    RUNTIME_HOOK_ENUM(rtMultipleTaskInfoLaunch),
    RUNTIME_HOOK_ENUM(rtSetDeviceEx),
    RUNTIME_HOOK_ENUM(rtDvppMalloc),
    RUNTIME_HOOK_ENUM(rtKernelLaunch),
    RUNTIME_HOOK_ENUM(rtKernelLaunchWithHandle),
    RUNTIME_HOOK_ENUM(rtKernelLaunchWithHandleV2),
    RUNTIME_HOOK_ENUM(rtKernelLaunchWithFlag),
    RUNTIME_HOOK_ENUM(rtKernelLaunchWithFlagV2),
    RUNTIME_HOOK_ENUM(rtKernelLaunchEx),
    RUNTIME_HOOK_ENUM(rtKernelLaunchFwk),
    RUNTIME_HOOK_ENUM(rtAicpuKernelLaunchWithFlag),
    RUNTIME_HOOK_ENUM(rtLaunchKernelByFuncHandle),
    RUNTIME_HOOK_ENUM(rtLaunchKernelByFuncHandleV2),
    RUNTIME_HOOK_ENUM(rtVectorCoreKernelLaunchWithHandle),
    RUNTIME_HOOK_ENUM(rtVectorCoreKernelLaunch),
    RUNTIME_HOOK_ENUM(rtStarsTaskLaunch),
    RUNTIME_HOOK_ENUM(rtStarsTaskLaunchWithFlag),
    RUNTIME_HOOK_ENUM(rtModelExecuteAsync),
    RUNTIME_HOOK_ENUM(rtModelExecuteSync),
    RUNTIME_HOOK_ENUM(rtsModelExecuteAsync),
    RUNTIME_HOOK_ENUM(rtsEventCreate),
    RUNTIME_HOOK_ENUM(rtsEventCreateEx),
    RUNTIME_HOOK_ENUM(rtNotifyRecord),
    RUNTIME_HOOK_ENUM(rtNotifyDestroy),
    RUNTIME_HOOK_ENUM(rtStreamWaitEventWithTimeout),
    RUNTIME_HOOK_ENUM(rtEventDestroySync),
    RUNTIME_HOOK_ENUM(rtNotifyCreate),
    RUNTIME_HOOK_ENUM(rtNotifyCreateWithFlag),
    RUNTIME_HOOK_ENUM(rtNotifyWait),
    RUNTIME_HOOK_ENUM(rtNotifyWaitWithTimeOut),
    RUNTIME_HOOK_ENUM(rtCntNotifyRecord),
    RUNTIME_HOOK_ENUM(rtCntNotifyWaitWithTimeout),
    RUNTIME_ENTRY_END,
} rt_hook_enum_t;

extern rt_entry_t rt_library_entry[];

extern void pre_rt_init(void);

#if defined(__cplusplus)
}
#endif

#endif