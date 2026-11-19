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

#include <acl/acl.h>
#include <gtest/gtest.h>
#include <runtime/rt.h>
#include <sys/file.h>
#include <mockcpp/mockcpp.hpp>
#include "dcmi_wrapper.h"
#include "log.h"
#include "mem_limiter.h"
#include "npu_manager.h"
#include "runtime_stub.h"
#include "securec.h"
#include "swap_hook.h"

extern "C" {
static int stub_memory_resource_info(int logic_id, int card_id, int device_id, size_t *used)
{
    (void)logic_id;
    (void)card_id;
    (void)device_id;
    *used = 0;
    return ENPU_SUCCESS;
}

int stub_get_mem_used_huge(size_t *used)
{
    *used = SIZE_MAX;
    return ENPU_SUCCESS;
}
}

namespace {
/* get_shm_state 打桩返回的伪共享内存状态; 同时桩化 swap_hook 相关函数, 不会被真实解引用 */
shm_state_t g_fake_shm_state;
swap_hook_t g_fake_swap_hook;
} // namespace

class MemoryTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        std::cout << "Memory test start" << std::endl;
    }

    static void TearDownTestCase()
    {
        std::cout << "Memory test end" << std::endl;
    }

    void SetUp()
    {
        (void)sprintf_s(g_log_config.log_dir, sizeof(g_log_config.log_dir), "%s", "../__build/log/enpu/");
        fd_ = open(stub_lock_path(), O_CREAT | O_RDONLY, 0755); // ut memctl.lock文件,设置为777权限
        MOCKER(lock_path).stubs().will(invoke(stub_lock_path));
        MOCKER(enpu_load_config).stubs().will(invoke(stub_enpu_load_config));
        MOCKER(enpu_dcmi_get_device_resource_info).stubs().will(invoke(stub_memory_resource_info));
        enpu_global_init();
        MOCKER(load_rt_libraries).stubs().will(invoke(stub_load_rt_libraries));
    }

    /* 打桩 get_shm_state 返回非空, 使拦截接口进入 swap 分支;
     * 同时桩化 swap_hook 全局句柄与换入检查, 避免访问未初始化的共享内存/执行器 */
    static void StubShmStateNonnull()
    {
        MOCKER(get_shm_state, shm_state_t * (void)).stubs().will(returnValue(&g_fake_shm_state));
        MOCKER(swap_hook_get_global, swap_hook_t * (void)).stubs().will(returnValue(&g_fake_swap_hook));
        MOCKER(swap_hook_check_and_swap_in, int(swap_hook_t *)).stubs().will(returnValue(ENPU_SUCCESS));
    }

    void TearDown()
    {
        GlobalMockObject::verify();
        GlobalMockObject::reset();
        close(fd_);
        fd_ = -1;
    }

private:
    int fd_ = -1;
};

TEST_F(MemoryTest, rtMalloc)
{
    constexpr uint32_t MAX_ARR_SIZE = 20;
    void *devPtrsArr[MAX_ARR_SIZE] = {nullptr};
    uint64_t size = 0;
    rtMemType_t type = 0;
    uint16_t moduleId = 0;
    rtError_t error = rtMalloc(devPtrsArr, size, type, moduleId);
    EXPECT_EQ(error, RT_ERROR_NONE);
    size = MAX_ARR_SIZE;
    error = rtMalloc(devPtrsArr, size, type, moduleId);
    EXPECT_EQ(error, ACL_ERROR_STORAGE_OVER_LIMIT);
}

TEST_F(MemoryTest, aclrtMallocImpl)
{
    constexpr uint32_t MAX_ARR_SIZE = 20;
    void *devPtrsArr[MAX_ARR_SIZE] = {nullptr};
    uint64_t size = 0;
    aclrtMemMallocPolicy policy = 0;
    rtError_t error = aclrtMallocImpl(devPtrsArr, size, policy);
    EXPECT_EQ(error, RT_ERROR_NONE);
    size = MAX_ARR_SIZE;
    error = aclrtMallocImpl(devPtrsArr, size, policy);
    EXPECT_EQ(error, ACL_ERROR_STORAGE_OVER_LIMIT);
}

TEST_F(MemoryTest, aclrtMallocAlign32Impl)
{
    constexpr uint32_t MAX_ARR_SIZE = 20;
    void *devPtrsArr[MAX_ARR_SIZE] = {nullptr};
    uint64_t size = 0;
    aclrtMemMallocPolicy policy = 0;
    rtError_t error = aclrtMallocAlign32Impl(devPtrsArr, size, policy);
    EXPECT_EQ(error, RT_ERROR_NONE);
    size = MAX_ARR_SIZE;
    error = aclrtMallocAlign32Impl(devPtrsArr, size, policy);
    EXPECT_EQ(error, ACL_ERROR_STORAGE_OVER_LIMIT);
}

TEST_F(MemoryTest, rtMallocCached)
{
    constexpr uint32_t MAX_ARR_SIZE = 20;
    void *devPtrsArr[MAX_ARR_SIZE] = {nullptr};
    uint64_t size = 0;
    rtMemType_t type = 0;
    uint16_t moduleId = 0;
    rtError_t error = rtMallocCached(devPtrsArr, size, type, moduleId);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMallocCachedImpl)
{
    constexpr uint32_t MAX_ARR_SIZE = 20;
    void *devPtrsArr[MAX_ARR_SIZE] = {nullptr};
    uint64_t size = 0;
    aclrtMemMallocPolicy policy = 0;
    rtError_t error = aclrtMallocCachedImpl(devPtrsArr, size, policy);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtDvppMalloc)
{
    constexpr uint32_t MAX_ARR_SIZE = 20;
    void *devPtrsArr[MAX_ARR_SIZE] = {nullptr};
    uint64_t size = 0;
    uint16_t moduleId = 0;
    rtError_t error = rtDvppMalloc(devPtrsArr, size, moduleId);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtDvppMallocWithFlag)
{
    constexpr uint32_t MAX_ARR_SIZE = 20;
    void *devPtrsArr[MAX_ARR_SIZE] = {nullptr};
    uint64_t size = 0;
    uint16_t moduleId = 0;
    uint32_t flag = 1;
    rtError_t error = rtDvppMallocWithFlag(devPtrsArr, size, flag, moduleId);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMemAlloc)
{
    constexpr uint32_t MAX_ARR_SIZE = 20;
    void *devPtrsArr[MAX_ARR_SIZE] = {nullptr};
    rtMallocPolicy policy = RT_MEM_MALLOC_HUGE_FIRST;
    rtMallocAdvise advise = RT_MEM_ADVISE_DVPP;
    rtMallocConfig_t *cfg = nullptr;
    uint64_t size = 0;
    rtError_t error = rtMemAlloc(devPtrsArr, size, policy, advise, cfg);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMemAllocManaged)
{
    constexpr uint32_t MAX_ARR_SIZE = 20;
    void *devPtrsArr[MAX_ARR_SIZE] = {nullptr};
    uint64_t size = 0;
    uint16_t moduleId = 0;
    uint32_t flag = 1;
    rtError_t error = rtMemAllocManaged(devPtrsArr, size, flag, moduleId);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemAllocManagedImpl)
{
    constexpr uint32_t MAX_ARR_SIZE = 20;
    void *devPtrsArr[MAX_ARR_SIZE] = {nullptr};
    uint64_t size = 0;
    uint32_t flag = 1;
    rtError_t error = aclrtMemAllocManagedImpl(devPtrsArr, size, flag);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMallocPhysical)
{
    rtDrvMemHandle *handle = nullptr;
    rtDrvMemProp_t *prop = nullptr;
    uint64_t size = 0;
    uint64_t flags = 1;
    rtError_t error = rtMallocPhysical(handle, size, prop, flags);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

// Non-device memory skips the swap path and goes straight to the driver.
TEST_F(MemoryTest, rtMallocPhysicalNonDeviceSide)
{
    rtDrvMemHandle handle = nullptr;
    rtDrvMemProp_t prop;
    prop.side = 0; // non-device, skip swap path
    uint64_t size = 0;
    uint64_t flags = 1;
    rtError_t error = rtMallocPhysical(&handle, size, &prop, flags);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMallocPhysicalImpl)
{
    rtDrvMemHandle *handle = nullptr;
    const aclrtPhysicalMemProp *prop = nullptr;
    uint64_t size = 0;
    uint64_t flags = 1;
    rtError_t error = aclrtMallocPhysicalImpl(handle, size, prop, flags);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMemGetInfoEx)
{
    rtMemInfoType_t memInfoType = RT_MEMORYINFO_HBM;
    size_t freeSize = 0;
    size_t totalSize = 1;
    rtError_t error = rtMemGetInfoEx(memInfoType, &freeSize, &totalSize);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtGetMemInfoImpl)
{
    aclrtMemAttr memInfoType = 0;
    size_t freeSize = 0;
    size_t totalSize = 1;
    rtError_t error = aclrtGetMemInfoImpl(memInfoType, &freeSize, &totalSize);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMemGetInfoEx_FailTest)
{
    static constexpr int32_t RT_ERROR_INVALID_VALUE = 0x07110001;
    MOCKER(get_mem_used, int(size_t *)).stubs().will(returnValue(-1));
    rtMemInfoType_t memInfoType = RT_MEMORYINFO_HBM;
    size_t freeSize = 0;
    size_t totalSize = 1;
    rtError_t error = rtMemGetInfoEx(memInfoType, &freeSize, &totalSize);
    EXPECT_EQ(error, RT_ERROR_INVALID_VALUE);
}

TEST_F(MemoryTest, aclrtGetMemInfoImpl_FailTest)
{
    static constexpr int32_t RT_ERROR_INVALID_VALUE = 0x07110001;
    MOCKER(get_mem_used, int(size_t *)).stubs().will(returnValue(-1));
    aclrtMemAttr memInfoType = 0;
    size_t freeSize = 0;
    size_t totalSize = 1;
    rtError_t error = aclrtGetMemInfoImpl(memInfoType, &freeSize, &totalSize);
    EXPECT_EQ(error, RT_ERROR_INVALID_VALUE);
}

TEST_F(MemoryTest, rtMemGetInfoEx_NullParams)
{
    rtMemInfoType_t memInfoType = RT_MEMORYINFO_HBM;
    size_t size = 0;
    EXPECT_EQ(rtMemGetInfoEx(memInfoType, nullptr, &size), RT_ERROR_INVALID_VALUE);
    EXPECT_EQ(rtMemGetInfoEx(memInfoType, &size, nullptr), RT_ERROR_INVALID_VALUE);
    EXPECT_EQ(rtMemGetInfoEx(memInfoType, nullptr, nullptr), RT_ERROR_INVALID_VALUE);
}

TEST_F(MemoryTest, rtMemGetInfoEx_UsedExceedsQuota)
{
    MOCKER(get_mem_used, int(size_t *)).stubs().will(invoke(stub_get_mem_used_huge));
    rtMemInfoType_t memInfoType = RT_MEMORYINFO_HBM;
    size_t freeSize = 0;
    size_t totalSize = 0;
    EXPECT_EQ(rtMemGetInfoEx(memInfoType, &freeSize, &totalSize), RT_ERROR_INVALID_VALUE);
}

// ===== mem-swap 扩展: memory.c 新增 RUNTIME_HOOK_DEFINE 接口 UT =====

TEST_F(MemoryTest, rtMemsetD32)
{
    EXPECT_EQ(rtMemsetD32(nullptr, 0, 0, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMemsetD32Async)
{
    EXPECT_EQ(rtMemsetD32Async(nullptr, 0, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMemSetAccess)
{
    EXPECT_EQ(rtMemSetAccess(nullptr, 0, nullptr, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMemGetAccess)
{
    EXPECT_EQ(rtMemGetAccess(nullptr, nullptr, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMemRetainAllocationHandle)
{
    EXPECT_EQ(rtMemRetainAllocationHandle(nullptr, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMemGetAddressRange)
{
    EXPECT_EQ(rtMemGetAddressRange(nullptr, nullptr, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMemMapSelectedLink)
{
    EXPECT_EQ(rtMemMapSelectedLink(nullptr, 0, nullptr, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtCmoAsync)
{
    EXPECT_EQ(rtCmoAsync(nullptr, 0, nullptr, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtsCmoAsync)
{
    EXPECT_EQ(rtsCmoAsync(nullptr, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtsCmoAsyncWithBarrier)
{
    EXPECT_EQ(rtsCmoAsyncWithBarrier(nullptr, 0, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtsLaunchCmoAddrTask)
{
    EXPECT_EQ(rtsLaunchCmoAddrTask(nullptr, nullptr, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtFreeImpl)
{
    EXPECT_EQ(aclrtFreeImpl(nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtFreePhysicalImpl)
{
    EXPECT_EQ(aclrtFreePhysicalImpl(nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMapMemImpl)
{
    EXPECT_EQ(aclrtMapMemImpl(nullptr, 0, 0, nullptr, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtUnmapMemImpl)
{
    EXPECT_EQ(aclrtUnmapMemImpl(nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpyImpl)
{
    EXPECT_EQ(aclrtMemcpyImpl(nullptr, 0, nullptr, 0, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtPointerGetAttributesImpl)
{
    EXPECT_EQ(aclrtPointerGetAttributesImpl(nullptr, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpyAsyncImpl)
{
    EXPECT_EQ(aclrtMemcpyAsyncImpl(nullptr, 0, nullptr, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtCheckMemTypeImpl)
{
    EXPECT_EQ(aclrtCheckMemTypeImpl(nullptr, 0, 0, nullptr, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpyAsyncWithConditionImpl)
{
    EXPECT_EQ(aclrtMemcpyAsyncWithConditionImpl(nullptr, 0, nullptr, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpyBatchImpl)
{
    EXPECT_EQ(aclrtMemcpyBatchImpl(nullptr, nullptr, nullptr, nullptr, 0, nullptr, nullptr, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpyBatchV2Impl)
{
    EXPECT_EQ(aclrtMemcpyBatchV2Impl(nullptr, nullptr, nullptr, nullptr, 0, nullptr, nullptr, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpyBatchAsyncImpl)
{
    EXPECT_EQ(aclrtMemcpyBatchAsyncImpl(nullptr, nullptr, nullptr, nullptr, 0, nullptr, nullptr, 0, nullptr, nullptr),
              RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpyBatchAsyncV2Impl)
{
    EXPECT_EQ(aclrtMemcpyBatchAsyncV2Impl(nullptr, nullptr, nullptr, nullptr, 0, nullptr, nullptr, 0, nullptr),
              RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpy2dImpl)
{
    EXPECT_EQ(aclrtMemcpy2dImpl(nullptr, 0, nullptr, 0, 0, 0, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpy2dAsyncImpl)
{
    EXPECT_EQ(aclrtMemcpy2dAsyncImpl(nullptr, 0, nullptr, 0, 0, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtSetMemcpyDescImpl)
{
    EXPECT_EQ(aclrtSetMemcpyDescImpl(nullptr, 0, nullptr, nullptr, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpyAsyncWithDescImpl)
{
    EXPECT_EQ(aclrtMemcpyAsyncWithDescImpl(nullptr, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpyAsyncWithOffsetImpl)
{
    EXPECT_EQ(aclrtMemcpyAsyncWithOffsetImpl(nullptr, 0, 0, nullptr, 0, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemsetImpl)
{
    EXPECT_EQ(aclrtMemsetImpl(nullptr, 0, 0, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemsetAsyncImpl)
{
    EXPECT_EQ(aclrtMemsetAsyncImpl(nullptr, 0, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemsetD32Impl)
{
    EXPECT_EQ(aclrtMemsetD32Impl(nullptr, 0, 0, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemsetD32AsyncImpl)
{
    EXPECT_EQ(aclrtMemsetD32AsyncImpl(nullptr, 0, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemP2PMapImpl)
{
    EXPECT_EQ(aclrtMemP2PMapImpl(nullptr, 0, 0, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtIpcMemGetExportKeyImpl)
{
    EXPECT_EQ(aclrtIpcMemGetExportKeyImpl(nullptr, 0, nullptr, 0, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtIpcMemImportByKeyImpl)
{
    EXPECT_EQ(aclrtIpcMemImportByKeyImpl(nullptr, nullptr, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtValueWriteImpl)
{
    EXPECT_EQ(aclrtValueWriteImpl(nullptr, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtValueWaitImpl)
{
    EXPECT_EQ(aclrtValueWaitImpl(nullptr, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemSetAccessImpl)
{
    EXPECT_EQ(aclrtMemSetAccessImpl(nullptr, 0, nullptr, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemGetAccessImpl)
{
    EXPECT_EQ(aclrtMemGetAccessImpl(nullptr, nullptr, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemRetainAllocationHandleImpl)
{
    EXPECT_EQ(aclrtMemRetainAllocationHandleImpl(nullptr, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemGetAddressRangeImpl)
{
    EXPECT_EQ(aclrtMemGetAddressRangeImpl(nullptr, nullptr, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemMapSelectedLinkImpl)
{
    EXPECT_EQ(aclrtMemMapSelectedLinkImpl(nullptr, 0, nullptr, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtCmoAsyncImpl)
{
    EXPECT_EQ(aclrtCmoAsyncImpl(nullptr, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtCmoAsyncWithBarrierImpl)
{
    EXPECT_EQ(aclrtCmoAsyncWithBarrierImpl(nullptr, 0, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtCmoAsyncWithDescImpl)
{
    EXPECT_EQ(aclrtCmoAsyncWithDescImpl(nullptr, 0, nullptr, nullptr), RT_ERROR_NONE);
}

// ===== get_shm_state() != NULL 分支: 打桩共享内存状态非空, 走 swap 换入/换出路径 =====

TEST_F(MemoryTest, rtMemsetD32ShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(rtMemsetD32(nullptr, 0, 0, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMemsetD32AsyncShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(rtMemsetD32Async(nullptr, 0, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMemSetAccessShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(rtMemSetAccess(nullptr, 0, nullptr, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMemGetAccessShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(rtMemGetAccess(nullptr, nullptr, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMemRetainAllocationHandleShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(rtMemRetainAllocationHandle(nullptr, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMemGetAddressRangeShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(rtMemGetAddressRange(nullptr, nullptr, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMemMapSelectedLinkShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(rtMemMapSelectedLink(nullptr, 0, nullptr, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtCmoAsyncShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(rtCmoAsync(nullptr, 0, nullptr, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtsCmoAsyncShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(rtsCmoAsync(nullptr, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtsCmoAsyncWithBarrierShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(rtsCmoAsyncWithBarrier(nullptr, 0, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtsLaunchCmoAddrTaskShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(rtsLaunchCmoAddrTask(nullptr, nullptr, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtFreeImplShmStateNonnull)
{
    StubShmStateNonnull();
    MOCKER(swap_hook_free_mem, int(swap_hook_t *, void *)).stubs().will(returnValue(ENPU_SUCCESS));
    EXPECT_EQ(aclrtFreeImpl(nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtFreePhysicalImplShmStateNonnull)
{
    StubShmStateNonnull();
    MOCKER(swap_hook_free_physical_mem, int(swap_hook_t *, rtDrvMemHandle)).stubs().will(returnValue(ENPU_SUCCESS));
    EXPECT_EQ(aclrtFreePhysicalImpl(nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMapMemImplShmStateNonnull)
{
    StubShmStateNonnull();
    MOCKER(swap_hook_map_mem, int(swap_hook_t *, void *, size_t, size_t, rtDrvMemHandle *, uint64_t))
        .stubs()
        .will(returnValue(ENPU_SUCCESS));
    EXPECT_EQ(aclrtMapMemImpl(nullptr, 0, 0, nullptr, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtUnmapMemImplShmStateNonnull)
{
    StubShmStateNonnull();
    MOCKER(swap_hook_unmap_mem, int(swap_hook_t *, void *)).stubs().will(returnValue(ENPU_SUCCESS));
    EXPECT_EQ(aclrtUnmapMemImpl(nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpyImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemcpyImpl(nullptr, 0, nullptr, 0, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtPointerGetAttributesImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtPointerGetAttributesImpl(nullptr, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpyAsyncImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemcpyAsyncImpl(nullptr, 0, nullptr, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtCheckMemTypeImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtCheckMemTypeImpl(nullptr, 0, 0, nullptr, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpyAsyncWithConditionImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemcpyAsyncWithConditionImpl(nullptr, 0, nullptr, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpyBatchImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemcpyBatchImpl(nullptr, nullptr, nullptr, nullptr, 0, nullptr, nullptr, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpyBatchV2ImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemcpyBatchV2Impl(nullptr, nullptr, nullptr, nullptr, 0, nullptr, nullptr, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpyBatchAsyncImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemcpyBatchAsyncImpl(nullptr, nullptr, nullptr, nullptr, 0, nullptr, nullptr, 0, nullptr, nullptr),
              RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpyBatchAsyncV2ImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemcpyBatchAsyncV2Impl(nullptr, nullptr, nullptr, nullptr, 0, nullptr, nullptr, 0, nullptr),
              RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpy2dImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemcpy2dImpl(nullptr, 0, nullptr, 0, 0, 0, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpy2dAsyncImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemcpy2dAsyncImpl(nullptr, 0, nullptr, 0, 0, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtSetMemcpyDescImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtSetMemcpyDescImpl(nullptr, 0, nullptr, nullptr, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpyAsyncWithDescImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemcpyAsyncWithDescImpl(nullptr, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemcpyAsyncWithOffsetImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemcpyAsyncWithOffsetImpl(nullptr, 0, 0, nullptr, 0, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemsetImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemsetImpl(nullptr, 0, 0, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemsetAsyncImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemsetAsyncImpl(nullptr, 0, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemsetD32ImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemsetD32Impl(nullptr, 0, 0, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemsetD32AsyncImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemsetD32AsyncImpl(nullptr, 0, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemP2PMapImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemP2PMapImpl(nullptr, 0, 0, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtIpcMemGetExportKeyImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtIpcMemGetExportKeyImpl(nullptr, 0, nullptr, 0, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtIpcMemImportByKeyImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtIpcMemImportByKeyImpl(nullptr, nullptr, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtValueWriteImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtValueWriteImpl(nullptr, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtValueWaitImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtValueWaitImpl(nullptr, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemSetAccessImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemSetAccessImpl(nullptr, 0, nullptr, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemGetAccessImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemGetAccessImpl(nullptr, nullptr, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemRetainAllocationHandleImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemRetainAllocationHandleImpl(nullptr, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemGetAddressRangeImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemGetAddressRangeImpl(nullptr, nullptr, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtMemMapSelectedLinkImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtMemMapSelectedLinkImpl(nullptr, 0, nullptr, 0), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtCmoAsyncImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtCmoAsyncImpl(nullptr, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtCmoAsyncWithBarrierImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtCmoAsyncWithBarrierImpl(nullptr, 0, 0, 0, nullptr), RT_ERROR_NONE);
}

TEST_F(MemoryTest, aclrtCmoAsyncWithDescImplShmStateNonnull)
{
    StubShmStateNonnull();
    EXPECT_EQ(aclrtCmoAsyncWithDescImpl(nullptr, 0, nullptr, nullptr), RT_ERROR_NONE);
}

// ===== get_shm_state() != NULL 分支: swap 换入/换出失败时透传错误码 =====

TEST_F(MemoryTest, rtMemSetAccessSwapInFail)
{
    constexpr int32_t SWAP_FAIL = -1;
    MOCKER(get_shm_state, shm_state_t * (void)).stubs().will(returnValue(&g_fake_shm_state));
    MOCKER(swap_hook_get_global, swap_hook_t * (void)).stubs().will(returnValue(&g_fake_swap_hook));
    MOCKER(swap_hook_check_and_swap_in, int(swap_hook_t *)).stubs().will(returnValue(SWAP_FAIL));
    EXPECT_EQ(rtMemSetAccess(nullptr, 0, nullptr, 0), SWAP_FAIL);
}

TEST_F(MemoryTest, aclrtFreeImplSwapFreeFail)
{
    constexpr int32_t SWAP_FAIL = -1;
    MOCKER(get_shm_state, shm_state_t * (void)).stubs().will(returnValue(&g_fake_shm_state));
    MOCKER(swap_hook_get_global, swap_hook_t * (void)).stubs().will(returnValue(&g_fake_swap_hook));
    MOCKER(swap_hook_check_and_swap_in, int(swap_hook_t *)).stubs().will(returnValue(ENPU_SUCCESS));
    MOCKER(swap_hook_free_mem, int(swap_hook_t *, void *)).stubs().will(returnValue(SWAP_FAIL));
    EXPECT_EQ(aclrtFreeImpl(nullptr), SWAP_FAIL);
}
