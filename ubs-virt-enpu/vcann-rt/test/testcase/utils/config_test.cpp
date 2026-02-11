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
#include <sys/mman.h>
#include <runtime/rt.h>
#include <acl/acl.h>
#include "securec.h"
#include "runtime_stub.h"
#include "log.h"
#include "config.h"
#include "npu_manager.h"
#include "mem_limiter.h"

class ConfigTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        std::cout<<"Config test start"<<std::endl;
    }

    static void TearDownTestCase()
    {
        std::cout<<"Config test end"<<std::endl;
    }

    void SetUp()
    {
        (void)sprintf_s(g_log_config.log_dir, sizeof(g_log_config.log_dir), "%s", "../__build/log/enpu/");
        open(stub_lock_path(), O_CREAT | O_RDONLY, 777); // ut memctl.lock文件,设置为777权限
        MOCKER(lock_path).stubs().will(invoke(stub_lock_path));
        MOCKER(enpu_load_config).stubs().will(invoke(stub_enpu_load_config));
        enpu_global_init();
        MOCKER(load_rt_libraries).stubs().will(invoke(stub_load_rt_libraries));
    }

    void TearDown()
    {
        GlobalMockObject::verify();
        GlobalMockObject::reset();
    }
};

TEST_F(ConfigTest, LoadConfigTest)
{
    int rc = load_config(MOCK_NPU_CONFIG_PATH);
    EXPECT_EQ(rc, RT_ERROR_NONE);
    rc = load_config(nullptr);
    EXPECT_EQ(rc, ENPU_FAIL);
}

TEST_F(ConfigTest, CheckInt32Test)
{
    int32_t option = -1;
    int rc = check_int32(-1, nullptr);
    EXPECT_EQ(rc, ENPU_FAIL);
}

TEST_F(ConfigTest, CheckStrTest)
{
    int32_t option = -1;
    std::string str = "";
    int rc = check_str(str.c_str(), nullptr);
    EXPECT_EQ(rc, ENPU_FAIL);
}
