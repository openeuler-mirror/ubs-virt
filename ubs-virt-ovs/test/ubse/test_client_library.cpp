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

#include "test_client_library.h"
#include "client_library.cpp"

namespace ovs::ut {

void TestClientLibrary::SetUp() {}

void TestClientLibrary::TearDown() {}

TEST_F(TestClientLibrary, Instance_ReturnsSingleton)
{
    int fakeHandleData = 0;
    void *fakeHandle = &fakeHandleData;
    MOCKER(dlopen).stubs().with(any(), any()).will(returnValue(fakeHandle));
    MOCKER(dlclose).stubs().with(any()).will(returnValue(0));
    
    auto &inst1 = ClientLibrary::Instance("/tmp/libtest.so");
    auto &inst2 = ClientLibrary::Instance("/tmp/libtest.so");
    EXPECT_EQ(&inst1, &inst2);
    
    GlobalMockObject::verify();
    MOCKER(dlopen).reset();
    MOCKER(dlclose).reset();
}

TEST_F(TestClientLibrary, GetSymbol_Success)
{
    int fakeHandleData = 0;
    void *fakeHandle = &fakeHandleData;
    int fakeSymbolData = 0;
    void *fakeSymbol = &fakeSymbolData;
    
    MOCKER(dlopen).stubs().with(any(), any()).will(returnValue(fakeHandle));
    MOCKER(dlsym).stubs().with(any(), any()).will(returnValue(fakeSymbol));
    MOCKER(dlclose).stubs().with(any()).will(returnValue(0));
    
    auto &lib = ClientLibrary::Instance("/tmp/libtest.so");
    void *sym = lib.GetSymbol("test_symbol");
    EXPECT_NE(sym, nullptr);
    
    GlobalMockObject::verify();
    MOCKER(dlopen).reset();
    MOCKER(dlsym).reset();
    MOCKER(dlclose).reset();
}

TEST_F(TestClientLibrary, GetSymbol_OpenFailed)
{
    MOCKER(dlopen).stubs().with(any(), any()).will(returnValue(nullptr));
    MOCKER(dlerror).stubs().will(returnValue("mock error"));
    
    auto &lib = ClientLibrary::Instance("/nonexistent.so");
    EXPECT_THROW(lib.GetSymbol("test_symbol"), std::runtime_error);
    
    GlobalMockObject::verify();
    MOCKER(dlopen).reset();
    MOCKER(dlerror).reset();
}

TEST_F(TestClientLibrary, GetSymbol_SymbolNotFound)
{
    int fakeHandleData = 0;
    void *fakeHandle = &fakeHandleData;
    
    MOCKER(dlopen).stubs().with(any(), any()).will(returnValue(fakeHandle));
    MOCKER(dlsym).stubs().with(any(), any()).will(returnValue(nullptr));
    MOCKER(dlerror).stubs().will(returnValue("symbol not found"));
    MOCKER(dlclose).stubs().with(any()).will(returnValue(0));
    
    auto &lib = ClientLibrary::Instance("/tmp/libtest.so");
    EXPECT_THROW(lib.GetSymbol("missing_symbol"), std::runtime_error);
    
    GlobalMockObject::verify();
    MOCKER(dlopen).reset();
    MOCKER(dlsym).reset();
    MOCKER(dlerror).reset();
    MOCKER(dlclose).reset();
}

} // namespace ovs::ut