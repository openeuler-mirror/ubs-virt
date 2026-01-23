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

namespace ovs::ut {

using namespace virt::ovs::ubse::client;

void TestClientLibrary::SetUp()
{
    Test::SetUp();
    client = std::make_shared<ClientLibrary>("/usr/lib64/libubse-client.so");
}

void TestClientLibrary::TearDown()
{
    Test::TearDown();
}

TEST(ClientLibraryTest, ConstructAndOpen)
{
    int fakeHandleData = 0;
    void *fakeHandle = &fakeHandleData;
    MOCKER(dlopen).stubs().with(any(), any()).will(returnValue(fakeHandle));
    auto &lib = ClientLibrary::Instance("/tmp/libc.so");
}
} // namespace ovs::ut
