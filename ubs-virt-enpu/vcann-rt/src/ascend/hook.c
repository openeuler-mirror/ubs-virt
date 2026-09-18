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

#include "runtime_hook.h"

rt_entry_t rt_library_entry[] = {
    /* aclrt及对应rt接口，cann-9.2.0，共73个 */
    {.name = "rtSetDevice"}, // runtime标识待废弃
    {.name = "aclrtSetDeviceImpl"},
    {.name = "rtSetDeviceWithoutTsd"}, // runtime仓间接口
    {.name = "aclrtSetDeviceWithoutTsdVXXImpl"},
    {.name = "rtGetSocVersion"}, // 仅内部调用，不拦截，runtime标识待废弃

    {.name = "rtMalloc"}, // runtime标识待废弃
    {.name = "aclrtMallocImpl"},
    {.name = "aclrtMallocAlign32Impl"},
    {.name = "rtMallocCached"}, // runtime标识待废弃
    {.name = "aclrtMallocCachedImpl"},
    {.name = "rtMemAllocManaged"}, // runtime标识待废弃
    {.name = "aclrtMemAllocManagedImpl"},
    {.name = "rtMallocPhysical"}, // runtime仓间接口
    {.name = "aclrtMallocPhysicalImpl"},
    {.name = "rtMemGetInfoEx"}, // runtime标识待废弃
    {.name = "aclrtGetMemInfoImpl"},

    {.name = "rtLaunchKernelByFuncHandleV3"}, // runtime标识待废弃
    {.name = "aclrtLaunchKernelImpl"},
    {.name = "rtsLaunchKernelWithHostArgs"}, // runtime标识待废弃
    {.name = "aclrtLaunchKernelWithHostArgsImpl"},
    {.name = "rtsLaunchKernelWithConfig"}, // runtime标识待废弃
    {.name = "aclrtLaunchKernelWithConfigImpl"},
    {.name = "rtsLaunchKernelWithDevArgs"}, // runtime标识待废弃
    {.name = "aclrtLaunchKernelV2Impl"},
    {.name = "rtsLaunchRandomNumTask"}, // runtime标识待废弃
    {.name = "aclrtRandomNumAsyncImpl"},
    {.name = "rtsLaunchReduceAsyncTask"}, // runtime标识待废弃
    {.name = "aclrtReduceAsyncImpl"},
    {.name = "rtsLaunchUpdateTask"}, // runtime标识待废弃
    {.name = "aclrtTaskUpdateAsyncImpl"},
    {.name = "rtLaunchKernelWithArgsArray"},            // cann-9.2.0新增
    {.name = "aclrtLaunchKernelWithArgsArrayImpl"},     // cann-9.2.0新增
    {.name = "rtLaunchSIMTKernelWithHostArgs"},         // cann-9.2.0新增
    {.name = "aclrtLaunchSIMTKernelWithHostArgsImpl"},  // cann-9.2.0新增
    {.name = "rtLaunchSIMTKernelWithArgsArray"},        // cann-9.2.0新增
    {.name = "aclrtLaunchSIMTKernelWithArgsArrayImpl"}, // cann-9.2.0新增
    {.name = "rtModelExecute"},                         // runtime标识待废弃
    {.name = "aclmdlRIExecuteAsyncImpl"},
    {.name = "rtStreamBeginCapture"}, // runtime标识待废弃
    {.name = "aclmdlRICaptureBeginImpl"},
    {.name = "rtStreamEndCapture"}, // runtime标识待废弃
    {.name = "aclmdlRICaptureEndImpl"},
    {.name = "rtsModelExecute"}, // runtime标识待废弃
    {.name = "aclmdlRIExecuteImpl"},
    {.name = "rtsStreamBeginTaskGrp"},
    {.name = "aclmdlRICaptureTaskGrpBeginImpl"},
    {.name = "rtsStreamEndTaskGrp"},
    {.name = "aclmdlRICaptureTaskGrpEndImpl"},
    {.name = "rtsStreamBeginTaskUpdate"},
    {.name = "aclmdlRICaptureTaskUpdateBeginImpl"},
    {.name = "rtsStreamEndTaskUpdate"},
    {.name = "aclmdlRICaptureTaskUpdateEndImpl"},
    {.name = "rtStreamGetCaptureInfo"}, // 仅内部调用，不拦截
    {.name = "rtModelGetStreams"},      // 仅内部调用，不拦截
    {.name = "rtStreamGetTasks"},       // 仅内部调用，不拦截

    {.name = "rtEventCreate"}, // runtime标识待废弃
    {.name = "aclrtCreateEventImpl"},
    {.name = "rtEventCreateWithFlag"}, // runtime标识待废弃
    {.name = "aclrtCreateEventWithFlagImpl"},
    {.name = "rtEventCreateExWithFlag"}, // runtime标识待废弃
    {.name = "aclrtCreateEventExWithFlagImpl"},
    {.name = "rtStreamWaitEvent"}, // runtime标识待废弃
    {.name = "aclrtStreamWaitEventImpl"},
    {.name = "rtEventRecord"}, // runtime标识待废弃
    {.name = "aclrtRecordEventImpl"},
    {.name = "aclrtRecordEventWithFlagImpl"},
    {.name = "rtEventElapsedTime"}, // 仅内部调用，不拦截
    {.name = "rtEventDestroy"},     // runtime标识待废弃
    {.name = "aclrtDestroyEventImpl"},
    {.name = "rtsNotifyCreate"}, // runtime标识待废弃
    {.name = "aclrtCreateNotifyImpl"},
    {.name = "rtsNotifyWaitAndReset"}, // runtime标识待废弃
    {.name = "aclrtWaitAndResetNotifyImpl"},
    {.name = "rtCntNotifyCreate"},         // runtime标识待废弃
    {.name = "rtCntNotifyCreateWithFlag"}, // runtime标识待废弃
    {.name = "aclrtCntNotifyCreateImpl"},
    {.name = "rtCntNotifyDestroy"}, // runtime标识待废弃
    {.name = "aclrtCntNotifyDestroyImpl"},
    {.name = "rtsCntNotifyRecord"},
    {.name = "aclrtCntNotifyRecordImpl"},
    {.name = "rtsCntNotifyWaitWithTimeout"},
    {.name = "aclrtCntNotifyWaitWithTimeoutImpl"},

    {.name = "rtStreamSynchronize"}, // 仅内部调用，不拦截，runtime标识待废弃
    {.name = "rtStreamDestroy"},     // runtime标识待废弃
    {.name = "aclrtDestroyStream"},

    /* rt仓间接口，cann-9.2.0，共8个 */
    {.name = "rtSetDeviceWithFlags"},

    {.name = "rtDvppMallocWithFlag"},
    {.name = "rtMemAlloc"}, // runtime标识待废弃

    {.name = "rtCpuKernelLaunchWithFlag"},
    {.name = "rtAicpuKernelLaunchExWithArgs"},
    {.name = "rtFftsPlusTaskLaunch"},
    {.name = "rtFftsPlusTaskLaunchWithFlag"},
    {.name = "rtMultipleTaskInfoLaunch"},

    /* 废弃rt/rts接口，cann-9.2.0，共31个 */
    {.name = "rtSetDeviceEx"},

    {.name = "rtDvppMalloc"},

    {.name = "rtKernelLaunch"},
    {.name = "rtKernelLaunchWithHandle"},
    {.name = "rtKernelLaunchWithHandleV2"},
    {.name = "rtKernelLaunchWithFlag"},
    {.name = "rtKernelLaunchWithFlagV2"},
    {.name = "rtKernelLaunchEx"},
    {.name = "rtKernelLaunchFwk"},
    {.name = "rtAicpuKernelLaunchWithFlag"},
    {.name = "rtLaunchKernelByFuncHandle"},
    {.name = "rtLaunchKernelByFuncHandleV2"},
    {.name = "rtVectorCoreKernelLaunchWithHandle"},
    {.name = "rtVectorCoreKernelLaunch"},
    {.name = "rtStarsTaskLaunch"},
    {.name = "rtStarsTaskLaunchWithFlag"},
    {.name = "rtModelExecuteAsync"},
    {.name = "rtModelExecuteSync"},
    {.name = "rtsModelExecuteAsync"},

    {.name = "rtsEventCreate"},
    {.name = "rtsEventCreateEx"},
    {.name = "rtNotifyRecord"},
    {.name = "rtNotifyDestroy"},
    {.name = "rtStreamWaitEventWithTimeout"},
    {.name = "rtEventDestroySync"},
    {.name = "rtNotifyCreate"},
    {.name = "rtNotifyCreateWithFlag"},
    {.name = "rtNotifyWait"},
    {.name = "rtNotifyWaitWithTimeOut"},
    {.name = "rtCntNotifyRecord"},
    {.name = "rtCntNotifyWaitWithTimeout"},
};