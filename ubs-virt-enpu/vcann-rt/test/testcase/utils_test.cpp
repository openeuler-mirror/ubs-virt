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

class UtilsTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        std::cout<<"Utils test start"<<std::endl;
    }

    static void TearDownTestCase()
    {
        std::cout<<"Utils test end"<<std::endl;
    }

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

TEST_F(UtilsTest, UtilsMemFailedTest)
{
    char *shmIdNullptr = nullptr;
    size_t size = 0;
    map_share_mem(shmIdNullptr, size);
    MOCKER(mmap, void *(void *, size_t, int, int, int, off_t)).stubs().will(returnValue(MAP_FAILED));
    char *shmIdNew = "Id";
    map_share_mem(shmIdNew, size);
    char *pathNullptr = nullptr;
    int operation = 1;
    file_lock resLock = file_lock_create(pathNullptr, operation);
    EXPECT_EQ(resLock.held, false);
    MOCKER(flock, int(int, int)).stubs().will(returnValue(-1));
    MOCKER(close, int(int)).stubs().will(returnValue(-1));
    file_lock *lockNullptr = nullptr;
    file_lock_destroy(lockNullptr);
    file_lock normalLock = { 1, true };
    file_lock_destroy(&normalLock);
    normalLock.held = false;
    file_lock_destroy(&normalLock);
}
