/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * ubs-virt-ovs is licensed under Mulan PSL v2.
 */
#include "test_urma_utility.h"

#define private public
#include "urma_utility.h"
#undef private

namespace ovs::ut
{
using namespace virt::ovs::ubse::urma;

static int g_urmaFakeHandle = 0;

static uint32_t MockGetBw(const char*, uint32_t* minBw, uint32_t* maxBw)
{
    if(minBw) *minBw = 10;
    if(maxBw) *maxBw = 100;
    return 0;
}
static uint32_t MockSetBw(const char*, uint32_t, uint32_t)
{ return 0; }
static uint32_t MockResetBw(const char*)
{ return 0; }

static void* MockDlsym(void* handle, const char* symbol)
{
    if (std::string(symbol) == "ubs_urma_bandwidth_get") return (void*)MockGetBw;
    if (std::string(symbol) == "ubs_urma_bandwidth_set") return (void*)MockSetBw;
    if (std::string(symbol) == "ubs_urma_bandwidth_reset") return (void*)MockResetBw;
    return nullptr;
}

void TestUrmaUtility::SetUp()
{
    MOCKER(dlopen).stubs().with(any(), any()).will(returnValue((void*)&g_urmaFakeHandle));
    
    // Instead of mockcpp string matching which is causing issues, map dlsym to a mock function
    MOCKER(dlsym).stubs().will(invoke(MockDlsym));
}

void TestUrmaUtility::TearDown()
{
    GlobalMockObject::verify();
    GlobalMockObject::reset();
}

TEST_F(TestUrmaUtility, GetBandWidth_Success)
{
    UrmaUtility utility;
    uint32_t minBw = 0, maxBw = 0;
    EXPECT_EQ(utility.GetBandWidth("dev", minBw, maxBw), 0u);
    EXPECT_EQ(minBw, 10u);
    EXPECT_EQ(maxBw, 100u);
}

TEST_F(TestUrmaUtility, SetBandWidth_Success)
{
    UrmaUtility utility;
    EXPECT_EQ(utility.SetBandWidth("dev", 10, 100), 0u);
}

TEST_F(TestUrmaUtility, ResetBandWidth_Success)
{
    UrmaUtility utility;
    EXPECT_EQ(utility.ResetBandWidth("dev"), 0u);
}
}
