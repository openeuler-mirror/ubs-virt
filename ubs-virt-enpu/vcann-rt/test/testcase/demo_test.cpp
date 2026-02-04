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
#include "securec.h"
#include "runtime_stub.h"
#include "runtime_stub.h"
#include "log.h"

class DemoTest : public testing::Test {
protected:
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

TEST_F(DemoTest, DemoTestCase)
{
    EXPECT_EQ(rtSetDevice(0), RT_ERROR_NONE);
}
