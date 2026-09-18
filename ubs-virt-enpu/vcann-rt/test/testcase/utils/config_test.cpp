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

#include "config.h"
#include <acl/acl.h>
#include <gtest/gtest.h>
#include <runtime/rt.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <mockcpp/mockcpp.hpp>
#include "log.h"
#include "mem_limiter.h"
#include "npu_manager.h"
#include "runtime_stub.h"
#include "securec.h"

class ConfigTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        std::cout << "Config test start" << std::endl;
    }

    static void TearDownTestCase()
    {
        std::cout << "Config test end" << std::endl;
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

TEST_F(ConfigTest, LoadConfigTest)
{
    int rc = load_config(MOCK_NPU_CONFIG_PATH);
    EXPECT_EQ(rc, RT_ERROR_NONE);
    rc = load_config(nullptr);
    EXPECT_EQ(rc, ENPU_FAIL);
}

TEST_F(ConfigTest, CheckInt32Test)
{
    int rc = check_int32(-1, nullptr);
    EXPECT_EQ(rc, ENPU_FAIL);
}

TEST_F(ConfigTest, CheckStrTest)
{
    std::string str = "";
    int rc = check_str(str.c_str(), nullptr);
    EXPECT_EQ(rc, ENPU_FAIL);
}

TEST_F(ConfigTest, CheckShmIdRejectsInvalid)
{
    EXPECT_EQ(check_shm_id("", OPTION_SHM_ID), ENPU_FAIL);
    EXPECT_EQ(check_shm_id("/tmp/shm", OPTION_SHM_ID), ENPU_FAIL);
    EXPECT_EQ(check_shm_id("shm/id", OPTION_SHM_ID), ENPU_FAIL);
    EXPECT_EQ(check_shm_id("..", OPTION_SHM_ID), ENPU_FAIL);
    EXPECT_EQ(check_shm_id("../shm", OPTION_SHM_ID), ENPU_FAIL);
}

TEST_F(ConfigTest, CheckShmIdAcceptsValid)
{
    EXPECT_EQ(check_shm_id("AAAAAAAA-BBBBBBBB", OPTION_SHM_ID), ENPU_SUCCESS);
    EXPECT_EQ(check_shm_id("shm.id.with.dots", OPTION_SHM_ID), ENPU_SUCCESS);
}

// A value whose length equals ret_len must be rejected (no room for the NUL).
TEST_F(ConfigTest, LoadStrBoundaryRejected)
{
    constexpr size_t kBufLen = 8;
    char buf[kBufLen] = {0};
    EXPECT_EQ(load_str(OPTION_SHM_ID, "1234567", buf, kBufLen), ENPU_SUCCESS);
    EXPECT_EQ(load_str(OPTION_SHM_ID, "12345678", buf, kBufLen), ENPU_FAIL);
}

// check_shm_id is wired into load_config: a path-traversal shm-id fails the load.
TEST_F(ConfigTest, LoadConfigRejectsInvalidShmId)
{
    const char *path = "../__build/bad_shm_id.config";
    const char *content = "physical-npu-id=0\n"
                          "virtual-npu-id=0\n"
                          "aicore-quota=50\n"
                          "memory-quota=32768\n"
                          "scheduling-policy=1\n"
                          "shm-id=../evil\n";
    FILE *f = fopen(path, "w");
    ASSERT_NE(f, nullptr);
    ASSERT_EQ(fputs(content, f) >= 0, true);
    ASSERT_EQ(fclose(f), 0);

    EXPECT_EQ(load_config(path), ENPU_FAIL);
    remove(path);

    // Restore the global config so later fixtures see a valid one.
    ASSERT_EQ(load_config(MOCK_NPU_CONFIG_PATH), ENPU_SUCCESS);
}
