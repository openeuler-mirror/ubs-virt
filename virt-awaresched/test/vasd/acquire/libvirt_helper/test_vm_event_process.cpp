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

TEST_F(TestVmEventProcess, RunAndStopTest)
{
    MOCKER(&LibvirtHelper::RunEventDefaultImpl).stubs().will(returnValue(VAS_ERROR));
    EXPECT_NO_THROW(VmEventProcess::Run());
    MOCKER(&LibvirtHelper::RunEventDefaultImpl).reset();
    EXPECT_NO_THROW(VmEventProcess::Stop());
}

VasRet MockGetVmInfo(virDomainPtr domain, VmInfo &vmInfo)
{
    vmInfo.uuid = UUID;
    vmInfo.name = VM_NAME;
    vmInfo.ioThreadMap[IO_THREAD_ID] = IoThreadInfo{};
    vmInfo.vcpuMap[0] = VcpuInfo{};
    return VAS_OK;
}

void MockGetIOThreads(const pid_t &pid, IoThread2PidMap &ioThread2PidMap)
{
    ioThread2PidMap =  {
        {IO_THREAD_ID, pid + 1},
    };
}

TEST_F(TestVmEventProcess, EventCallbackTest_StartEvent)
{
    MOCKER(LibvirtHelper::GetVmInfo).stubs().will(invoke(MockGetVmInfo));
    Vm2VcpuMap vmVcpuPidList;
    vmVcpuPidList[UUID] = {{0, PID + 1}};
    MOCKER(&ProcHelper::GetVmProcList).stubs().will(returnValue(vmVcpuPidList));
    ProcHelper::GetInstance().uuid2PidMap[UUID] = PID;
    MOCKER(ProcHelper::GetIOThreads).stubs().will(invoke(MockGetIOThreads));

    int event = static_cast<int>(virDomainEventType::VIR_DOMAIN_EVENT_STARTED);
    int detail = static_cast<int>(virDomainEventStartedDetailType::VIR_DOMAIN_EVENT_STARTED_BOOTED);
    virDomainPtr domain = reinterpret_cast<virDomainPtr>(0x1234);
    int result = VmEventProcess::EventCallback(nullptr, domain, event, detail, nullptr);
    ProcHelper::GetInstance().uuid2PidMap.clear();

    EXPECT_EQ(result, static_cast<int>(VAS_OK));
    VmEventInfo vmEventInfo{};
    EXPECT_EQ(VmEventProcess::Pop(vmEventInfo), VAS_OK);
    EXPECT_EQ(vmEventInfo.eventType, VmEventType::START);
    EXPECT_EQ(vmEventInfo.vmInfo.uuid, UUID);
    EXPECT_EQ(vmEventInfo.vmInfo.name, VM_NAME);
    EXPECT_EQ(vmEventInfo.vmInfo.tgid, PID);
    EXPECT_EQ(vmEventInfo.vmInfo.vcpuMap[0].pid, PID + 1);
    EXPECT_EQ(vmEventInfo.vmInfo.ioThreadMap[IO_THREAD_ID].pid, PID + 1);
}

VasRet MockGetVmUUID(virDomainPtr domain, std::string &uuid)
{
    uuid = UUID;
    return VAS_OK;
}

TEST_F(TestVmEventProcess, EventCallbackTest_ShutdownEvent)
{
    MOCKER(LibvirtHelper::GetVmUUID).stubs().will(invoke(MockGetVmUUID));

    int event = static_cast<int>(virDomainEventType::VIR_DOMAIN_EVENT_SHUTDOWN);
    int detail = static_cast<int>(virDomainEventShutdownDetailType::VIR_DOMAIN_EVENT_SHUTDOWN_HOST);
    virDomainPtr domain = reinterpret_cast<virDomainPtr>(0x1234);
    int result = VmEventProcess::EventCallback(nullptr, domain, event, detail, nullptr);
    EXPECT_EQ(result, static_cast<int>(VAS_OK));

    VmEventInfo vmEventInfo;
    EXPECT_EQ(VmEventProcess::Pop(vmEventInfo), VAS_OK);
    EXPECT_EQ(vmEventInfo.eventType, VmEventType::SHUTDOWN);
    EXPECT_EQ(vmEventInfo.vmInfo.uuid, UUID);
}

TEST_F(TestVmEventProcess, EventCallbackTest_OtherCondition)
{
    int event = static_cast<int>(virDomainEventType::VIR_DOMAIN_EVENT_SHUTDOWN);
    int detail = static_cast<int>(virDomainEventShutdownDetailType::VIR_DOMAIN_EVENT_SHUTDOWN_GUEST);
    virDomainPtr domain = reinterpret_cast<virDomainPtr>(0x1234);
    int result = VmEventProcess::EventCallback(nullptr, domain, event, detail, nullptr);
    EXPECT_EQ(result, static_cast<int>(VAS_OK));
}
}