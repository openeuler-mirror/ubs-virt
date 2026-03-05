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

#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>
#include <sys/file.h>
#include <runtime/rt.h>
#include "securec.h"
#include "runtime_stub.h"
#include "log.h"
#include "npu_manager.h"
#include "mem_limiter.h"

class CoreLimiterTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        std::cout<<"Core Limiter test start"<<std::endl;
    }

    static void TearDownTestCase()
    {
        std::cout<<"Core Limiter test end"<<std::endl;
    }

    void SetUp()
    {
        (void)sprintf_s(g_log_config.log_dir, sizeof(g_log_config.log_dir), "%s", "../__build/log/enpu/");
        fd_ = open(stub_lock_path(), O_CREAT | O_RDONLY, 0755); // ut中的memctl.lock文件,设置为755权限
        MOCKER(lock_path).stubs().will(invoke(stub_lock_path));
        MOCKER(enpu_load_config).stubs().will(invoke(stub_enpu_load_config));
        enpu_global_init();
        MOCKER(load_rt_libraries).stubs().will(invoke(stub_load_rt_libraries));
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

TEST_F(CoreLimiterTest, npu_utilization_monitor_thread_test)
{
    void *para = nullptr;
    (void)npu_utilization_monitor_thread(para);
    int owner = 0;
    check_and_borrow_timeslice(owner);
    bool ret = slide_window_check(owner);
    EXPECT_EQ(ret, false);
    owner = get_vnpu_id();
    check_and_borrow_timeslice(owner);
    ret = slide_window_check(owner);
    EXPECT_EQ(ret, false);
}

TEST_F(CoreLimiterTest, calculate_alive_vnpu_num_test)
{
    int32_t count = calculate_alive_vnpu_num();
    EXPECT_EQ(count, 1);
}
