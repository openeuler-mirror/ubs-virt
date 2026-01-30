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

#include "runtime_hook.h"

rt_entry_t rt_library_entry[] = {
    /* Init Part */
    {.name = "rtSetDevice"},
    {.name = "rtSetDeviceEx"},
    {.name = "rtSetDeviceWithFlags"},
    /* Memory Part */
    {.name = "rtMalloc"},
    {.name = "rtMallocCached"},
    {.name = "rtDvppMalloc"},
    {.name = "rtDvppMallocWithFlag"},
    {.name = "rtMemAlloc"},
    {.name = "rtMemAllocManaged"},
    {.name = "rtMallocPhysical"},
    /* Kernel Part */
    {.name = "rtKernelLaunch"},
    {.name = "rtKernelLaunchWithHandle"},
    {.name = "rtKernelLaunchWithHandleV2"},
    {.name = "rtKernelLaunchWithFlag"},
    {.name = "rtKernelLaunchWithFlagV2"},
    {.name = "rtKernelLaunchEx"},
    {.name = "rtKernelLaunchFwk"},
    {.name = "rtCpuKernelLaunch"},
    {.name = "rtCpuKernelLaunchWithFlag"},
    {.name = "rtAicpuKernelLaunchWithFlag"},
    {.name = "rtAicpuKernelLaunchExWithArgs"},
    {.name = "rtLaunchKernelByFuncHandle"},
    {.name = "rtLaunchKernelByFuncHandleV2"},
    {.name = "rtLaunchKernelByFuncHandleV3"},
    {.name = "rtVectorCoreKernelLaunchWithHandle"},
    {.name = "rtVectorCoreKernelLaunch"},

    {.name = "rtFftsPlusTaskLaunch"},
    {.name = "rtFftsPlusTaskLaunchWithFlag"},
    {.name = "rtFftsTaskLaunch"},
    {.name = "rtFftsTaskLaunchWithFlag"},
    {.name = "rtModelExecute"},
    {.name = "rtModelExecuteAsync"},
    {.name = "rtStreamBeginCapture"},
    {.name = "rtStreamEndCapture"},
    {.name = "rtsModelExecute"},
    {.name = "rtModelExecuteSync"},
    {.name = "rtStarsTaskLaunch"},
    {.name = "rtStarsTaskLaunchWithFlag"},
    {.name = "rtCmoTaskLaunch"},
    {.name = "rtCmoAddrTaskLaunch"},
    {.name = "rtBarrierTaskLaunch"},
    {.name = "rtMultipleTaskInfoLaunch"},
    {.name = "rtMultipleTaskInfoLaunchWithFlag"},
    /* Event Part */
    {.name = "rtEventCreate"},
    {.name = "rtsEventCreate"},
    {.name = "rtsEventCreateEx"},
    {.name = "rtEventCreateWithFlag"},
    {.name = "rtEventCreateExWithFlag"},
    {.name = "rtStreamWaitEvent"},
    {.name = "rtEventRecord"},
    {.name = "rtEventDestroy"},
    {.name = "rtEventReset"},
    {.name = "rtsNotifyCreate"},
    {.name = "rtNotifyRecord"},
    {.name = "rtNotifyDestroy"},
    {.name = "rtsNotifyWaitAndReset"},
    /* Other Part */
    {.name = "rtStreamSynchronize"},
    {.name = "rtStreamDestroy"},
    {.name = "rtDestroyStreamForce"},
};