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

#include "test_urma_utility.h"
#include "urma_utility.cpp"

namespace ovs::ut {

void TestUrmaUtility::SetUp() {}

void TestUrmaUtility::TearDown() {}

static uint32_t MockBandWidthGet(const char *name, uint32_t *minBw, uint32_t *maxBw)
{
    if (minBw) *minBw = 100;
    if (maxBw) *maxBw = 1000;
    return 0;
}

static uint32_t MockBandWidthGetFailed(const char *name, uint32_t *minBw, uint32_t *maxBw)
{
    return 1;
}

static uint32_t MockBandWidthSet(const char *name, uint32_t minBw, uint32_t maxBw)
{
    return 0;
}

static uint32_t MockBandWidthSetFailed(const char *name, uint32_t minBw, uint32_t maxBw)
{
    return 1;
}

static uint32_t MockBandWidthReset(const char *name)
{
    return 0;
}

static uint32_t MockBandWidthResetFailed(const char *name)
{
    return 1;
}

TEST_F(TestUrmaUtility, Instance_ReturnsSingleton)
{
    int fakeHandleData = 0;
    void *fakeHandle = &fakeHandleData;
    
    MOCKER(dlopen).stubs().with(any(), any()).will(returnValue(fakeHandle));
    MOCKER(dlsym).stubs().with(any(), any()).will(returnValue((void *)MockBandWidthGet));
    MOCKER(dlclose).stubs().with(any()).will(returnValue(0));
    
    auto &inst1 = UrmaUtility::Instance();
    auto &inst2 = UrmaUtility::Instance();
    EXPECT_EQ(&inst1, &inst2);
    
    GlobalMockObject::verify();
    MOCKER(dlopen).reset();
    MOCKER(dlsym).reset();
    MOCKER(dlclose).reset();
}

TEST_F(TestUrmaUtility, GetBandWidth_Success)
{
    int fakeHandleData = 0;
    void *fakeHandle = &fakeHandleData;
    
    MOCKER(dlopen).stubs().with(any(), any()).will(returnValue(fakeHandle));
    MOCKER(dlsym).stubs().with(any(), any()).will(returnValue((void *)MockBandWidthGet));
    MOCKER(dlclose).stubs().with(any()).will(returnValue(0));
    
    auto &urma = UrmaUtility::Instance();
    uint32_t minBw = 0;
    uint32_t maxBw = 0;
    uint32_t ret = urma.GetBandWidth("test_device", minBw, maxBw);
    EXPECT_EQ(ret, 0u);
    EXPECT_EQ(minBw, 100u);
    EXPECT_EQ(maxBw, 1000u);
    
    GlobalMockObject::verify();
    MOCKER(dlopen).reset();
    MOCKER(dlsym).reset();
    MOCKER(dlclose).reset();
}

TEST_F(TestUrmaUtility, GetBandWidth_Failed)
{
    int fakeHandleData = 0;
    void *fakeHandle = &fakeHandleData;
    
    MOCKER(dlopen).stubs().with(any(), any()).will(returnValue(fakeHandle));
    MOCKER(dlsym).stubs().with(any(), any()).will(returnValue((void *)MockBandWidthGetFailed));
    MOCKER(dlclose).stubs().with(any()).will(returnValue(0));
    
    auto &urma = UrmaUtility::Instance();
    uint32_t minBw = 0;
    uint32_t maxBw = 0;
    uint32_t ret = urma.GetBandWidth("test_device", minBw, maxBw);
    EXPECT_NE(ret, 0u);
    
    GlobalMockObject::verify();
    MOCKER(dlopen).reset();
    MOCKER(dlsym).reset();
    MOCKER(dlclose).reset();
}

TEST_F(TestUrmaUtility, SetBandWidth_Success)
{
    int fakeHandleData = 0;
    void *fakeHandle = &fakeHandleData;
    
    MOCKER(dlopen).stubs().with(any(), any()).will(returnValue(fakeHandle));
    MOCKER(dlsym).stubs().with(any(), any()).will(returnValue((void *)MockBandWidthSet));
    MOCKER(dlclose).stubs().with(any()).will(returnValue(0));
    
    auto &urma = UrmaUtility::Instance();
    uint32_t ret = urma.SetBandWidth("test_device", 100, 1000);
    EXPECT_EQ(ret, 0u);
    
    GlobalMockObject::verify();
    MOCKER(dlopen).reset();
    MOCKER(dlsym).reset();
    MOCKER(dlclose).reset();
}

TEST_F(TestUrmaUtility, SetBandWidth_Failed)
{
    int fakeHandleData = 0;
    void *fakeHandle = &fakeHandleData;
    
    MOCKER(dlopen).stubs().with(any(), any()).will(returnValue(fakeHandle));
    MOCKER(dlsym).stubs().with(any(), any()).will(returnValue((void *)MockBandWidthSetFailed));
    MOCKER(dlclose).stubs().with(any()).will(returnValue(0));
    
    auto &urma = UrmaUtility::Instance();
    uint32_t ret = urma.SetBandWidth("test_device", 100, 1000);
    EXPECT_NE(ret, 0u);
    
    GlobalMockObject::verify();
    MOCKER(dlopen).reset();
    MOCKER(dlsym).reset();
    MOCKER(dlclose).reset();
}

TEST_F(TestUrmaUtility, ResetBandWidth_Success)
{
    int fakeHandleData = 0;
    void *fakeHandle = &fakeHandleData;
    
    MOCKER(dlopen).stubs().with(any(), any()).will(returnValue(fakeHandle));
    MOCKER(dlsym).stubs().with(any(), any()).will(returnValue((void *)MockBandWidthReset));
    MOCKER(dlclose).stubs().with(any()).will(returnValue(0));
    
    auto &urma = UrmaUtility::Instance();
    uint32_t ret = urma.ResetBandWidth("test_device");
    EXPECT_EQ(ret, 0u);
    
    GlobalMockObject::verify();
    MOCKER(dlopen).reset();
    MOCKER(dlsym).reset();
    MOCKER(dlclose).reset();
}

TEST_F(TestUrmaUtility, ResetBandWidth_Failed)
{
    int fakeHandleData = 0;
    void *fakeHandle = &fakeHandleData;
    
    MOCKER(dlopen).stubs().with(any(), any()).will(returnValue(fakeHandle));
    MOCKER(dlsym).stubs().with(any(), any()).will(returnValue((void *)MockBandWidthResetFailed));
    MOCKER(dlclose).stubs().with(any()).will(returnValue(0));
    
    auto &urma = UrmaUtility::Instance();
    uint32_t ret = urma.ResetBandWidth("test_device");
    EXPECT_NE(ret, 0u);
    
    GlobalMockObject::verify();
    MOCKER(dlopen).reset();
    MOCKER(dlsym).reset();
    MOCKER(dlclose).reset();
}

} // namespace ovs::ut