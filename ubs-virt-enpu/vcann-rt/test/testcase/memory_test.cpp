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

#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>
#include <sys/file.h>
#include <runtime/rt.h>
#include <acl/acl.h>
#include "securec.h"
#include "runtime_stub.h"
#include "log.h"

class MemoryTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        std::cout<<"Memory test start"<<std::endl;
    }

    static void TearDownTestCase()
    {
        std::cout<<"Memory test end"<<std::endl;
    }

    void SetUp()
    {
        (void)sprintf_s(g_log_config.log_dir, sizeof(g_log_config.log_dir), "%s", "../build/log/enpu/");
        open(stub_lock_path(), O_CREAT | O_RDONLY, 777); // ut memctl.lock文件,设置为777权限
        enpu_global_init();
        MOCKER(load_rt_libraries).stubs().will(invoke(stub_load_rt_libraries));
        MOCKER(lock_path).stubs().will(invoke(stub_lock_path));
    }

    void TearDown()
    {
        GlobalMockObject::verify();
        GlobalMockObject::reset();
    }
};

TEST_F(MemoryTest, rtMalloc)
{
    constexpr uint32_t MAX_ARR_SIZE = 20;
    void* devPtrsArr[MAX_ARR_SIZE] = { nullptr };
    uint64_t size = 0;
    rtMemType_t type = 0;
    uint16_t moduleId = 0;
    rtError_t error = rtMalloc(devPtrsArr, size, type, moduleId);
    EXPECT_EQ(error, RT_ERROR_NONE);
    size = MAX_ARR_SIZE;
    error = rtMalloc(devPtrsArr, size, type, moduleId);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMallocCached)
{
    constexpr uint32_t MAX_ARR_SIZE = 20;
    void* devPtrsArr[MAX_ARR_SIZE] = { nullptr };
    uint64_t size = 0;
    rtMemType_t type = 0;
    uint16_t moduleId = 0;
    rtError_t error = rtMallocCached(devPtrsArr, size, type, moduleId);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtDvppMalloc)
{
    constexpr uint32_t MAX_ARR_SIZE = 20;
    void* devPtrsArr[MAX_ARR_SIZE] = { nullptr };
    uint64_t size = 0;
    uint16_t moduleId = 0;
    rtError_t error = rtDvppMalloc(devPtrsArr, size, moduleId);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtDvppMallocWithFlag)
{
    constexpr uint32_t MAX_ARR_SIZE = 20;
    void* devPtrsArr[MAX_ARR_SIZE] = { nullptr };
    uint64_t size = 0;
    uint16_t moduleId = 0;
    uint32_t flag = 1;
    rtError_t error = rtDvppMallocWithFlag(devPtrsArr, size, flag, moduleId);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(MemoryTest, rtMemAlloc)
{
    constexpr uint32_t MAX_ARR_SIZE = 20;
    void* devPtrsArr[MAX_ARR_SIZE] = { nullptr };
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
    void* devPtrsArr[MAX_ARR_SIZE] = { nullptr };
    uint64_t size = 0;
    uint16_t moduleId = 0;
    uint32_t flag = 1;
    rtError_t error = rtMemAllocManaged(devPtrsArr, size, flag, moduleId);
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

TEST_F(MemoryTest, rtMemGetInfoEx)
{
    rtMemInfoType_t memInfoType = 0;
    size_t freeSize = 0;
    size_t totalSize = 1;
    rtError_t error = rtMemGetInfoEx(memInfoType, &freeSize, &totalSize);
    EXPECT_EQ(error, RT_ERROR_NONE);
}
