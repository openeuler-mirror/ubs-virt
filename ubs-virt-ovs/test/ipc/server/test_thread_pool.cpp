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

#include "test_thread_pool.h"

using namespace virt::ovs::ipc::server;
using namespace virt::ovs;
namespace ovs::ut {
void TestThreadPool::SetUp()
{
    Test::SetUp();
}

void TestThreadPool::TearDown()
{
    Test::TearDown();
}

TEST_F(TestThreadPool, StartAndStop)
{
    ThreadPool pool(2);
    EXPECT_NO_THROW({
        pool.Start();
        pool.Stop();
    });
}

TEST_F(TestThreadPool, EnqueueTaskExecuted)
{
    ThreadPool pool(1);
    pool.Start();

    std::atomic<bool> executed{ false };
    std::mutex mtx;
    std::condition_variable cv;

    bool ok = pool.TryEnqueue([&] {
        executed = true;
        cv.notify_one();
    });

    EXPECT_TRUE(ok);
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait_for(lock, std::chrono::seconds(1), [&] { return executed.load(); });
    }

    EXPECT_TRUE(executed);
    pool.Stop();
}

TEST_F(TestThreadPool, QueueSizeCoverage)
{
    ThreadPool pool(1);

    EXPECT_EQ(pool.QueueSize(), 0);
}
}