/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * ubs-virt-ovs is licensed under Mulan PSL v2.
 */
#include "test_client_library.h"

#define private public
#include "client_library.h"
#undef private

namespace ovs::ut
{
using namespace virt::ovs::ubse::client;

static int g_fakeHandle = 0;

void TestClientLibrary::SetUp()
{}
void TestClientLibrary::TearDown()
{
    GlobalMockObject::verify();
    GlobalMockObject::reset();
}

TEST_F(TestClientLibrary, Open_Success)
{
    ClientLibrary lib("/dummy.so");
    MOCKER(dlopen).stubs().with(any(), any()).will(returnValue((void*)&g_fakeHandle));
    
    EXPECT_NO_THROW(lib.Open());
    EXPECT_EQ(lib.handle, &g_fakeHandle);
}

TEST_F(TestClientLibrary, Open_Failed)
{
    ClientLibrary lib("/dummy.so");
    MOCKER(dlopen).stubs().with(any(), any()).will(returnValue((void*)nullptr));
    MOCKER(dlerror).stubs().will(returnValue((char*)"mock error"));
    
    EXPECT_THROW(lib.Open(), std::runtime_error);
}

TEST_F(TestClientLibrary, GetSymbol_Success)
{
    ClientLibrary lib("/dummy.so");
    lib.handle = &g_fakeHandle;
    
    int fakeSymbol = 0;
    MOCKER(dlsym).stubs().with(any(), any()).will(returnValue((void*)&fakeSymbol));
    MOCKER(dlopen).stubs().with(any(), any()).will(returnValue((void*)&g_fakeHandle));

    void* sym = lib.GetSymbol("test_symbol");
    EXPECT_EQ(sym, &fakeSymbol);
}

TEST_F(TestClientLibrary, GetSymbol_NotFound)
{
    ClientLibrary lib("/dummy.so");
    MOCKER(dlopen).stubs().with(any(), any()).will(returnValue((void*)&g_fakeHandle));
    MOCKER(dlsym).stubs().with(any(), any()).will(returnValue((void*)nullptr));
    MOCKER(dlerror).stubs().will(returnValue((char*)"not found"));
    
    EXPECT_THROW(lib.GetSymbol("test_symbol"), std::runtime_error);
}
}
