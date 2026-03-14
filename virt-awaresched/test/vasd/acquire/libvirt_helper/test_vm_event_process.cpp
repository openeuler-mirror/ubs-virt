/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 *
 * VSched is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *      http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include "test_vm_event_process.h"

#include "vm_event_process.h"

namespace vas::ut::acquire {
using namespace vas::sched::acquire;
using namespace vas::sched;
static const std::string UUID = "3faa6728-6027-4bd1-a858-6107d2221428";
static const std::string VM_NAME = "test_vm";
static const pid_t PID = 1;
static const pid_t IO_THREAD_ID = 12345;
void TestVmEventProcess::SetUp()
{
    Test::SetUp();
}

void TestVmEventProcess::TearDown()
{
    GlobalMockObject::verify();
    Test::TearDown();
}
}