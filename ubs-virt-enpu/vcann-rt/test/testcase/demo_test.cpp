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
#include <runtime/rt.h>
#include "runtime_stub.h"

class DemoTest : public testing::Test {
protected:
    void SetUp()
    {
        MOCKER(load_rt_libraries).stubs().will(invoke(stub_load_rt_libraries));
    }
    void TearDown() {}
};

TEST_F(DemoTest, DemoTestCase)
{
    EXPECT_EQ(rtSetDevice(0), RT_ERROR_NONE);
}

TEST_F(DemoTest, rtMalloc)
{
    rtError_t error = rtSetDevice(0);
    EXPECT_EQ(error, RT_ERROR_NONE);
    void *srcAddr = nullptr;
    error = rtMalloc(&srcAddr, 1, 0, 0);
    EXPECT_EQ(error, RT_ERROR_NONE);
}
