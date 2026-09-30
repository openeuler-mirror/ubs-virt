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
#include "test_cluster_sched.h"

#include <mockcpp/mokc.h>

#include "cluster_sched.h"
#include "cpu_helper.h"
#include "error.h"
#include "vasd_arg_parse.h"

namespace vas::ut::sched {

using namespace vas::ut::sched;
inline constexpr int NUMBER_ZERO = 0;
inline constexpr int NUMBER_ONE = 1;
inline constexpr int NUMBER_TWO = 2;
inline constexpr int NUMBER_THREE = 3;
inline constexpr int NUMBER_TEN = 10;
void TestClusterSched::SetUp()
{
    Test::SetUp();
    ClusterSched::GetInstance().numaClusterMap_.clear();
    ClusterSched::GetInstance().domainMap_.clear();
    ClusterSched::GetInstance().entityMap_.clear();
    ClusterSched::GetInstance().groupMap_.clear();
    ClusterSched::GetInstance().compactionCount_ = NUMBER_ZERO;
    ClusterSched::GetInstance().overProvision_ = {{0, 1}, {1, 1}, {2, 1}, {3, 1}};
}

void TestClusterSched::TearDown()
{
    GlobalMockObject::verify();
    Test::TearDown();
}

std::string TestClusterSched::uuid01 = "a1d11347-8738-45fb-8944-e3a058f46401";
VmInfoMap TestClusterSched::oneVmInfo = VmInfoMap{{
    uuid01,
    VmInfo{
        .uuid = uuid01,
        .name = "vm01",
        .tgid = 266956,
        .vcpuMap =
            VcpuNumaMap{
                {0, VcpuInfo{266987, {0}, DynamicBitset{}}},
                {1, VcpuInfo{266988, {0}, DynamicBitset{}}},
                {2, VcpuInfo{266989, {0}, DynamicBitset{}}},
            },
    },
}};
NumaClusterMap TestClusterSched::defaultNumaClusterMap =
    NumaClusterMap{{0,
                    {{0, Cluster{.id = 0,
                                 .numaId = 0,
                                 .cpuSet = {0, 1, 2, 3, 4, 5, 6, 7},
                                 .clusterLayers = {ClusterLayer{
                                     .idle = 8,
                                     .total = 8,
                                     .usedBitmap = DynamicBitset(8),
                                     .groups = {},
                                 }}}}}}};
DomainMap TestClusterSched::defaultDomainMap{{
    uuid01 + "_0",
    {
        .uuid = uuid01,
        .numaId = 0,
        .tgid = 266956,
        .pidVcpuMap =
            PidVcpuMap{
                {266987, 0},
                {266988, 1},
                {266989, 2},
            },
        .groups = {uuid01 + "_0_0_0"},
    },
}};
EntityMap TestClusterSched::defaultEntityMap = EntityMap{
    {266987,
     VmEntity{
         .pid = 266987,
         .cpuIdx = 0,
     }},
    {266988,
     VmEntity{
         .pid = 266987,
         .cpuIdx = 1,
     }},
    {266989,
     VmEntity{
         .pid = 266987,
         .cpuIdx = 2,
     }},
};
GroupMap TestClusterSched::defaultGroupMap =
    GroupMap{{uuid01 + "_0_0_0", VmGroup{
                                     .domainKey = uuid01 + "_0",
                                     .id = uuid01 + "_0_0_0",
                                     .clusterId = 0,
                                     .layerId = 0,
                                     .start = 0,
                                     .nrCpus = 3,
                                     .usedBitmap = DynamicBitset(CpuHelper::CLUSTER_CPU_NUM, false),
                                     .entityPids = {266987, 266988, 266989},
                                 }}};
uint16_t TestClusterSched::defaultCompactionCount{};
uint8_t TestClusterSched::defaultOverProvision = 1;
CpuSet TestClusterSched::cluster0CpuList{0, 1, 2, 3, 4, 5, 6, 7};
CpuTopologyMap TestClusterSched::defaultCpuTopologyMap{{
    0,
    ClusterInfo{
        .id = 0,
        .numaId = 0,
        .total = 8,
        .cpuSet = std::set<uint16_t>{0, 1, 2, 3, 4, 5, 6, 7},
        .bitMap = DynamicBitset(8),
    },
}};

VasRet GetVmInfoListMockSuccess(LibvirtHelper *cls, VmInfoMap &vmInfoMap);

TEST_F(TestClusterSched, testUpdateDomainInfo1)
{
    MOCKER_CPP(&LibvirtHelper::GetVmInfoList, VasRet(LibvirtHelper::*)(VmInfoMap &))
        .stubs()
        .will(invoke(GetVmInfoListMockSuccess));
    MOCKER(&ClusterSched::SelectVmNuma).stubs().will(returnValue(VAS_OK));
    MOCKER(&ClusterSched::Alloc).stubs().will(returnValue(VAS_ERROR)).then(returnValue(VAS_OK));
    MOCKER(&ClusterSched::Assign).stubs().will(returnValue(VAS_ERROR)).then(returnValue(VAS_OK));
    std::string domainKey = uuid01 + "_0";
    EXPECT_EQ(ClusterSched::GetInstance().UpdateDomainInfosAndSched(), VAS_OK);
    EXPECT_FALSE(ClusterSched::GetInstance().domainMap_[domainKey].isReScheded);
    EXPECT_EQ(ClusterSched::GetInstance().UpdateDomainInfosAndSched(), VAS_OK);
    EXPECT_FALSE(ClusterSched::GetInstance().domainMap_[domainKey].isReScheded);
}

TEST_F(TestClusterSched, testUpdateDomainInfo2)
{
    MOCKER_CPP(&LibvirtHelper::GetVmInfoList, VasRet(LibvirtHelper::*)(VmInfoMap &))
        .stubs()
        .will(returnValue(VAS_ERROR));
    EXPECT_EQ(ClusterSched::GetInstance().UpdateDomainInfosAndSched(), VAS_ERROR);
}

TEST_F(TestClusterSched, testUpdateDomainInfo3)
{
    MOCKER_CPP(&LibvirtHelper::GetVmInfoList, VasRet(LibvirtHelper::*)(VmInfoMap &))
        .stubs()
        .will(invoke(GetVmInfoListMockSuccess));
    MOCKER(&ClusterSched::SelectVmNuma).stubs().will(returnValue(VAS_ERROR));
    MOCKER(&ClusterSched::Alloc).stubs().will(returnValue(VAS_ERROR)).then(returnValue(VAS_OK));
    MOCKER(&ClusterSched::Assign).stubs().will(returnValue(VAS_ERROR)).then(returnValue(VAS_OK));
    std::string domainKey = uuid01 + "_0";
    EXPECT_EQ(ClusterSched::GetInstance().UpdateDomainInfosAndSched(), VAS_OK);
}

// AI-03: the retry cycle must release residual groups of a previous failed Alloc/Assign
// before reallocating, so that a failing VM leaves no double reservation behind.
TEST_F(TestClusterSched, testUpdateDomainInfoRetryIdempotent)
{
    ClusterSched scheduler;
    const std::string domainKey = uuid01 + "_0";
    const std::string groupId = uuid01 + "_0_0_0";
    // Simulate the residual state of a previous failed cycle:
    // the domain holds group g1 which is still recorded in groupMap_ and the cluster layer.
    VmDomain residual{};
    residual.uuid = uuid01;
    residual.name = "vm01";
    residual.numaId = 0;
    residual.tgid = 266956;
    residual.pidVcpuMap = {{266987, 0}, {266988, 1}, {266989, 2}};
    residual.groups = {groupId};
    residual.isReScheded = false;
    scheduler.domainMap_ = {{domainKey, residual}};
    scheduler.groupMap_ = defaultGroupMap;
    scheduler.numaClusterMap_ = NumaClusterMap{
        {0,
         {{0, Cluster{.id = 0,
                      .numaId = 0,
                      .cpuSet = {0, 1, 2, 3, 4, 5, 6, 7},
                      .clusterLayers = {ClusterLayer{
                          .idle = 5, .total = 8, .usedBitmap = DynamicBitset(8), .groups = {groupId}}}}}}}};
    scheduler.overProvision_ = {{0, 1}};

    MOCKER_CPP(&LibvirtHelper::GetVmInfoList, VasRet(LibvirtHelper::*)(VmInfoMap &))
        .stubs()
        .will(invoke(GetVmInfoListMockSuccess));
    MOCKER(&ClusterSched::SelectVmNuma).stubs().will(returnValue(VAS_OK));
    // Alloc keeps failing: the retry must still have released the residual group first.
    MOCKER(&ClusterSched::Alloc).stubs().will(returnValue(VAS_ERROR));

    EXPECT_EQ(scheduler.UpdateDomainInfosAndSched(), VAS_OK);
    // The residual group is fully released, no reservation is left on the cluster layer.
    EXPECT_TRUE(scheduler.groupMap_.empty());
    EXPECT_TRUE(scheduler.domainMap_[domainKey].groups.empty());
    EXPECT_EQ(scheduler.numaClusterMap_[0][0].clusterLayers[0].idle, 8);
    EXPECT_TRUE(scheduler.numaClusterMap_[0][0].clusterLayers[0].groups.empty());
}

TEST_F(TestClusterSched, testSelectMinLayer)
{
    std::set<uint16_t> availableNumas = {0, 1, 3};
    uint16_t selectedNumaId = 0;
    VasRet ret = ClusterSched::GetInstance().SelectMinLayer(availableNumas, selectedNumaId);
    EXPECT_EQ(ret, VAS_OK);
}

TEST_F(TestClusterSched, testInitClusterInfo1)
{
    MOCKER(CpuHelper::GetClusterCpuSet).stubs().will(returnValue(cluster0CpuList));
    int cpuNum = 128;
    MOCKER(CpuHelper::GetMaxCpuNum).stubs().will(returnValue(cpuNum));
    MOCKER_CPP(&CpuHelper::GenCpuTopology, CpuTopologyMap(CpuHelper::*)())
        .stubs()
        .will(returnValue(defaultCpuTopologyMap));
    ClusterSched::GetInstance().InitClusterInfo();
    EXPECT_EQ(ClusterSched::GetInstance().numaClusterMap_.size(), NUMBER_ONE);
    EXPECT_EQ(ClusterSched::GetInstance().numaClusterMap_[NUMBER_ZERO].size(), NUMBER_ONE);
    EXPECT_EQ(ClusterSched::GetInstance().numaClusterMap_[NUMBER_ZERO][NUMBER_ZERO].id, NUMBER_ZERO);
    EXPECT_EQ(ClusterSched::GetInstance().numaClusterMap_[NUMBER_ZERO][NUMBER_ZERO].numaId, NUMBER_ZERO);
    int cpuSetSize = 8;
    EXPECT_EQ(ClusterSched::GetInstance().numaClusterMap_[NUMBER_ZERO][NUMBER_ZERO].cpuSet.size(), cpuSetSize);
    EXPECT_EQ(ClusterSched::GetInstance().numaClusterMap_[NUMBER_ZERO][NUMBER_ZERO].clusterLayers.size(), NUMBER_ONE);
    EXPECT_EQ(ClusterSched::GetInstance().numaClusterMap_[NUMBER_ZERO][NUMBER_ZERO].clusterLayers[NUMBER_ZERO].idle,
              cpuSetSize);
    EXPECT_EQ(ClusterSched::GetInstance().numaClusterMap_[NUMBER_ZERO][NUMBER_ZERO].clusterLayers[NUMBER_ZERO].total,
              cpuSetSize);
    EXPECT_EQ(
        ClusterSched::GetInstance().numaClusterMap_[NUMBER_ZERO][NUMBER_ZERO].clusterLayers[NUMBER_ZERO].groups.size(),
        NUMBER_ZERO);
}

TEST_F(TestClusterSched, testInitClusterInfo2)
{
    MOCKER_CPP(&CpuHelper::Init, VasRet(*)()).stubs().will(returnValue(VAS_ERROR));
    EXPECT_EQ(ClusterSched::GetInstance().InitClusterInfo(), VAS_ERROR);
}

TEST_F(TestClusterSched, testGetDomainsByUuid)
{
    ClusterSched::GetInstance().domainMap_ = defaultDomainMap;
    const auto ret = ClusterSched::GetInstance().GetDomainsByUuid(uuid01);
    EXPECT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0].uuid, uuid01);
}

TEST_F(TestClusterSched, testReSchedVm)
{
    MOCKER_CPP(&ClusterSched::ClusterCompactionWithoutLock, void (ClusterSched::*)()).stubs().will(ignoreReturnValue());
    MOCKER_CPP(&ClusterSched::Alloc, VasRet(ClusterSched::*)(VmDomain &))
        .stubs()
        .will(returnValue(VAS_ERROR))
        .then(returnValue(VAS_OK));
    MOCKER_CPP(&ClusterSched::Assign, VasRet(ClusterSched::*)(VmDomain &))
        .stubs()
        .will(returnValue(VAS_ERROR))
        .then(returnValue(VAS_OK));
    MOCKER_CPP(&ClusterSched::Unassign, void (ClusterSched::*)(VmDomain &)).stubs().will(returnValue(VAS_OK));
    MOCKER_CPP(&ClusterSched::Free, void (ClusterSched::*)(VmDomain &)).stubs().will(returnValue(VAS_OK));
    ClusterSched::GetInstance().domainMap_ = defaultDomainMap;
    ClusterSched::GetInstance().numaClusterMap_ = defaultNumaClusterMap;
    const std::string domainKey = uuid01 + "_0";
    auto ret = ClusterSched::GetInstance().ReSchedVm("all");
    EXPECT_EQ(ret, VAS_ERROR);
    ret = ClusterSched::GetInstance().ReSchedVm(uuid01);
    EXPECT_EQ(ret, VAS_ERROR);
    ret = ClusterSched::GetInstance().ReSchedVm(uuid01);
    EXPECT_EQ(ret, VAS_OK);
    ret = ClusterSched::GetInstance().ReSchedVm(uuid01);
    EXPECT_EQ(ret, VAS_OK);
}

TEST_F(TestClusterSched, testClusterCompaction)
{
    ClusterSched::GetInstance().domainMap_ = defaultDomainMap;
    ClusterSched::GetInstance().numaClusterMap_ = defaultNumaClusterMap;
    MOCKER_CPP(&ClusterSched::CleanDyingPid, VasRet(ClusterSched::*)()).stubs().will(returnValue(VAS_OK));
    MOCKER_CPP(&ClusterSched::CompactionCluster, void (ClusterSched::*)(uint16_t, std::map<uint16_t, Cluster> &))
        .stubs()
        .will(ignoreReturnValue());
    EXPECT_NO_THROW(ClusterSched::GetInstance().ClusterCompaction());
}

VasRet GetVmInfoListMockSuccess(LibvirtHelper *cls, VmInfoMap &vmInfoMap)
{
    vmInfoMap = VmInfoMap{{
        TestClusterSched::uuid01,
        VmInfo{
            .uuid = TestClusterSched::uuid01,
            .name = "vm01",
            .tgid = 266956,
            .ioThreadMap = {{1, IoThreadInfo{266990, DynamicBitset{}}}},
            .vcpuMap =
                VcpuNumaMap{
                    {0, VcpuInfo{266987, {0}, DynamicBitset{}}},
                    {1, VcpuInfo{266988, {0}, DynamicBitset{}}},
                    {2, VcpuInfo{266989, {0}, DynamicBitset{}}},
                },
        },
    }};
    return VAS_OK;
}

TEST_F(TestClusterSched, testReSchedStartedVms)
{
    ClusterSched::GetInstance().domainMap_ = defaultDomainMap;
    ClusterSched::GetInstance().numaClusterMap_ = defaultNumaClusterMap;
    MOCKER_CPP(&LibvirtHelper::GetVmInfoList, VasRet(LibvirtHelper::*)(VmInfoMap &))
        .stubs()
        .will(returnValue(VAS_ERROR))
        .then(invoke(GetVmInfoListMockSuccess));
    MOCKER_CPP(&ClusterSched::Alloc, VasRet(ClusterSched::*)(VmDomain &))
        .stubs()
        .will(returnValue(VAS_ERROR))
        .then(returnValue(VAS_OK));
    MOCKER_CPP(&ClusterSched::Assign, VasRet(ClusterSched::*)(VmDomain &))
        .stubs()
        .will(returnValue(VAS_ERROR))
        .then(returnValue(VAS_OK));
    EXPECT_NO_THROW(ClusterSched::GetInstance().ReSchedStartedVms());
    EXPECT_NO_THROW(ClusterSched::GetInstance().ReSchedStartedVms());
    EXPECT_NO_THROW(ClusterSched::GetInstance().ReSchedStartedVms());
}

void GetCacheMockSuccess(VmInfoMap &vmInfoMap)
{
    vmInfoMap = VmInfoMap{{
        TestClusterSched::uuid01,
        VmInfo{
            .uuid = TestClusterSched::uuid01,
            .name = "vm01",
            .tgid = 266956,
            .ioThreadMap = {{1, IoThreadInfo{266990, DynamicBitset{}}}},
            .vcpuMap =
                VcpuNumaMap{
                    {0, VcpuInfo{266987, {0}, DynamicBitset{}}},
                    {1, VcpuInfo{266988, {0}, DynamicBitset{}}},
                    {2, VcpuInfo{266989, {0}, DynamicBitset{}}},
                },
        },
    }};
}

TEST_F(TestClusterSched, testRecoverVmVcpu)
{
    ClusterSched::GetInstance().domainMap_ = defaultDomainMap;
    ClusterSched::GetInstance().numaClusterMap_ = defaultNumaClusterMap;
    MOCKER(ClusterSched::GetVmCgroupPath).stubs().will(returnValue(VAS_OK));
    MOCKER(ClusterSched::SetVmCpuset).stubs().will(returnValue(VAS_OK));
    EXPECT_NO_THROW(ClusterSched::GetInstance().RecoverVmVcpu(oneVmInfo));
}

TEST_F(TestClusterSched, testFree)
{
    ClusterSched::GetInstance().domainMap_ = defaultDomainMap;
    std::string domainKey = uuid01 + "_0";
    auto domain = ClusterSched::GetInstance().domainMap_[domainKey];
    ClusterSched::GetInstance().groupMap_ = defaultGroupMap;
    MOCKER_CPP(&ClusterSched::DelGroupFromClusterLayer, void (*)(const VmGroup &, ClusterLayer &))
        .stubs()
        .will(ignoreReturnValue());
    domain.groups.emplace(uuid01 + "_1_0_0");
    EXPECT_NO_THROW(ClusterSched::GetInstance().Free(domain));
    domain.groups.clear();
    domain.groups.emplace(uuid01 + "_0_0_0");
    ClusterSched::GetInstance().numaClusterMap_ = NumaClusterMap{{1,
                                                                  {{0, Cluster{.id = 0,
                                                                               .numaId = 0,
                                                                               .cpuSet = {0, 1, 2, 3, 4, 5, 6, 7},
                                                                               .clusterLayers = {ClusterLayer{
                                                                                   .idle = 8,
                                                                                   .total = 8,
                                                                                   .usedBitmap = DynamicBitset(8),
                                                                                   .groups = {},
                                                                               }}}}}}};
    EXPECT_NO_THROW(ClusterSched::GetInstance().Free(domain));
    ClusterSched::GetInstance().groupMap_ = defaultGroupMap;
    ClusterSched::GetInstance().numaClusterMap_ = defaultNumaClusterMap;
    EXPECT_NO_THROW(ClusterSched::GetInstance().Free(domain));
}

TEST_F(TestClusterSched, testAssign)
{
    ClusterSched::GetInstance().domainMap_ = defaultDomainMap;
    std::string domainKey = uuid01 + "_0";
    auto domain = ClusterSched::GetInstance().domainMap_[domainKey];
    ClusterSched::GetInstance().entityMap_ = defaultEntityMap;
    auto ret = ClusterSched::GetInstance().Assign(domain);
    EXPECT_EQ(ret, VAS_OK);
    ClusterSched::GetInstance().entityMap_.clear();
    MOCKER_CPP(&ClusterSched::AssignPidCpu, VasRet(ClusterSched::*)(VmDomain &, const pid_t &))
        .stubs()
        .will(returnValue(VAS_ERROR))
        .then(returnValue(VAS_OK));
    ret = ClusterSched::GetInstance().Assign(domain);
    EXPECT_EQ(ret, VAS_ERROR);
    ret = ClusterSched::GetInstance().Assign(domain);
    EXPECT_EQ(ret, VAS_OK);
}

TEST_F(TestClusterSched, testGetAffinityInfo)
{
    std::unordered_map<std::string, VmAffinity> ret{};
    ClusterSched::GetInstance().domainMap_ = defaultDomainMap;
    ClusterSched::GetInstance().groupMap_ = defaultGroupMap;
    ClusterSched::GetInstance().numaClusterMap_ = defaultNumaClusterMap;
    ClusterSched::GetInstance().entityMap_ = defaultEntityMap;
    ClusterSched::GetInstance().GetAffinityInfo(uuid01, ret);
    ASSERT_FALSE(ret.find(uuid01) == ret.end());
}

TEST_F(TestClusterSched, testUnAssignPidCpu)
{
    VmDomain vmDomain;
    int numaIdNum = 2;
    int tgidNum = 4567;
    vmDomain.uuid = "a1b2c3d4-e5f6-7890-abcd-ef1234567890";
    vmDomain.name = "ProductionVM-01";
    vmDomain.numaId = numaIdNum;
    vmDomain.tgid = tgidNum;
    vmDomain.pidVcpuMap = {{8001, 0}, {8002, 1}, {8003, 2}};
    vmDomain.groups = {"production", "highperformance", "webcluster"};
    vmDomain.entityPids = {4567, 4568, 4569};
    vmDomain.isReScheded = false;

    ClusterSched::GetInstance().UnAssignPidCpu(vmDomain);

    EXPECT_TRUE(ClusterSched::GetInstance().groupMap_["production"].entityPids.empty());
    EXPECT_TRUE(ClusterSched::GetInstance().groupMap_["webcluster"].entityPids.empty());
}

TEST_F(TestClusterSched, testUnAssignPidCpu2)
{
    VmDomain vmDomain;
    int numaIdNum = 2;
    int tgidNum = 4567;
    vmDomain.uuid = "a1b2c3d4-e5f6-7890-abcd-ef1234567890";
    vmDomain.name = "ProductionVM-01";
    vmDomain.numaId = numaIdNum;
    vmDomain.tgid = tgidNum;
    vmDomain.pidVcpuMap = {{8001, 0}, {8002, 1}, {8003, 2}};
    vmDomain.groups = {"production", "highperformance", "webcluster"};
    vmDomain.entityPids = {4567, 4568, 4569};
    vmDomain.isReScheded = false;

    MOCKER(Bitset::FindFirstIdlePos).stubs().will(returnValue(0));
    ClusterSched::GetInstance().UnAssignPidCpu(vmDomain);

    EXPECT_TRUE(ClusterSched::GetInstance().groupMap_["production"].entityPids.empty());
    EXPECT_TRUE(ClusterSched::GetInstance().groupMap_["webcluster"].entityPids.empty());
}

TEST_F(TestClusterSched, testGetGranularity)
{
    auto ret = ClusterSched::GetInstance().GetGranularity();
    EXPECT_EQ(ret, VAS_ERROR);
}

TEST_F(TestClusterSched, testAssignPidCpu)
{
    auto &clusterSched = ClusterSched::GetInstance();
    VmDomain vmDomain;
    int numaIdNum = 2;
    int tgidNum = 4567;
    vmDomain.uuid = "a1b2c3d4-e5f6-7890-abcd-ef1234567890";
    vmDomain.name = "ProductionVM-01";
    vmDomain.numaId = numaIdNum;
    vmDomain.tgid = tgidNum;
    vmDomain.pidVcpuMap = {{8001, 0}, {8002, 1}, {8003, 2}};
    vmDomain.groups = {"production", "highperformance", "webcluster"};
    vmDomain.entityPids = {4567, 4568, 4569};
    vmDomain.isReScheded = false;

    for (const auto &pid : vmDomain.entityPids) {
        VasRet result = clusterSched.AssignPidCpu(vmDomain, pid);
        EXPECT_EQ(result, VAS_ERROR);
    }
}

TEST_F(TestClusterSched, testGetNumaTotalCpus)
{
    auto ret = ClusterSched::GetInstance().GetNumaTotalCpus(16);
    EXPECT_EQ(ret, VAS_OK);
}

TEST_F(TestClusterSched, testAllocClusterGroupToDomain)
{
    auto &clusterSched = ClusterSched::GetInstance();
    VmDomain vmDomain;
    int numaIdNum = 2;
    int tgidNum = 4567;
    vmDomain.uuid = "a1b2c3d4-e5f6-7890-abcd-ef1234567890";
    vmDomain.name = "ProductionVM-01";
    vmDomain.numaId = numaIdNum;
    vmDomain.tgid = tgidNum;
    vmDomain.pidVcpuMap = {{8001, 0}, {8002, 1}, {8003, 2}};
    vmDomain.groups = {"production", "highperformance", "webcluster"};
    vmDomain.entityPids = {4567, 4568, 4569};
    vmDomain.isReScheded = false;

    VasRet result = clusterSched.AllocClusterGroupToDomain(vmDomain, 4567);
    EXPECT_EQ(result, VAS_OK);
}

// AI-03: Alloc must roll back all partial allocations when it fails midway
// (no enough consecutive cpu for the remaining vCPUs), leaving no residual records.
TEST_F(TestClusterSched, testAllocRollbackOnPartialFailure)
{
    ClusterSched scheduler;
    VasdArgParse::smt = true; // granularity = 1, so nr = pidVcpuMap.size() = 3
    VmDomain vmDomain;
    vmDomain.uuid = uuid01;
    vmDomain.name = "vm01";
    vmDomain.numaId = 0;
    vmDomain.tgid = 266956;
    vmDomain.pidVcpuMap = {{266987, 0}, {266988, 1}, {266989, 2}};
    vmDomain.isReScheded = false;

    // numa0 total = 2 + 4 = 6 >= nr(3): the first round allocates the whole cluster0 (2 cpus),
    // the second round finds no consecutive idle cpu anywhere, triggering the failure path.
    scheduler.numaClusterMap_ = NumaClusterMap{
        {0,
         {{0, Cluster{.id = 0,
                      .numaId = 0,
                      .cpuSet = {0, 1},
                      .clusterLayers = {ClusterLayer{.idle = 2, .total = 2, .usedBitmap = DynamicBitset(2)}}}},
          {1, Cluster{.id = 1,
                      .numaId = 0,
                      .cpuSet = {2, 3, 4, 5},
                      .clusterLayers = {ClusterLayer{.idle = 4, .total = 4, .usedBitmap = DynamicBitset(4)}}}}}}};
    scheduler.overProvision_ = {{0, 1}};
    // 1st call: cluster0 has a whole idle layer, allocation succeeds;
    // 2nd call: no consecutive idle cpu is found for the remaining 1 vCPU.
    MOCKER(Bitset::FindFirstIdlePos).stubs().will(returnValue(int16_t(0))).then(returnValue(int16_t(-1)));

    EXPECT_EQ(scheduler.Alloc(vmDomain), VAS_ERROR);
    // No residual allocation is left after the rollback.
    EXPECT_TRUE(scheduler.groupMap_.empty());
    EXPECT_TRUE(vmDomain.groups.empty());
    EXPECT_EQ(scheduler.numaClusterMap_[0][0].clusterLayers[0].idle, 2);
    EXPECT_TRUE(scheduler.numaClusterMap_[0][0].clusterLayers[0].groups.empty());
    EXPECT_EQ(scheduler.numaClusterMap_[0][1].clusterLayers[0].idle, 4);
    VasdArgParse::smt = false; // restore the global flag for the following test cases
}

TEST_F(TestClusterSched, testGenEntity)
{
    ClusterSched scheduler;
    pid_t myThreadPid = getpid();
    VmEntity &entity = scheduler.GenEntity(myThreadPid, 2);
    EXPECT_EQ(entity.pid, myThreadPid);
}

TEST_F(TestClusterSched, testCleanDyingPidByGroup)
{
    ClusterSched scheduler;
    VmDomain vmDomain;
    int numaIdNum = 2;
    int tgidNum = 4567;
    vmDomain.uuid = "a1b2c3d4-e5f6-7890-abcd-ef1234567890";
    vmDomain.name = "ProductionVM-01";
    vmDomain.numaId = numaIdNum;
    vmDomain.tgid = tgidNum;
    vmDomain.pidVcpuMap = {{8001, 0}, {8002, 1}, {8003, 2}};
    vmDomain.groups = {"production", "highperformance", "webcluster"};
    vmDomain.entityPids = {4567, 4568, 4569};
    vmDomain.isReScheded = false;
    VmEntity vmEntity;
    VmGroup vmGroup;
    vmEntity.pid = 0;
    vmEntity.cpuIdx = 0;
    scheduler.AddEntityToGroup(vmEntity, 0, vmGroup);
    scheduler.CleanDyingPidByGroup(vmDomain);
    EXPECT_TRUE(ClusterSched::GetInstance().groupMap_["production"].entityPids.empty());
    EXPECT_TRUE(ClusterSched::GetInstance().groupMap_["webcluster"].entityPids.empty());
}

TEST_F(TestClusterSched, testCleanDyingPidByGroup2)
{
    ClusterSched scheduler;
    ClusterSched::GetInstance().groupMap_ = {{"a1b2c3d4-e5f6-7890-abcd-ef1234567890_0_0_0", {}}};
    VmDomain vmDomain;
    int numaIdNum = 2;
    int tgidNum = 4567;
    vmDomain.uuid = "a1b2c3d4-e5f6-7890-abcd-ef1234567890";
    vmDomain.name = "ProductionVM-01";
    vmDomain.numaId = numaIdNum;
    vmDomain.tgid = tgidNum;
    vmDomain.pidVcpuMap = {{8001, 0}, {8002, 1}, {8003, 2}};
    vmDomain.groups = {"a1b2c3d4-e5f6-7890-abcd-ef1234567890_0_0_0"};
    vmDomain.entityPids = {4567, 4568, 4569};
    vmDomain.isReScheded = false;
    VmEntity vmEntity;
    VmGroup vmGroup;
    vmEntity.pid = 0;
    vmEntity.cpuIdx = 0;
    scheduler.AddEntityToGroup(vmEntity, 0, vmGroup);
    scheduler.CleanDyingPidByGroup(vmDomain);
    EXPECT_TRUE(ClusterSched::GetInstance().groupMap_["a1b2c3d4-e5f6-7890-abcd-ef1234567890_0_0_0"].entityPids.empty());
}

TEST_F(TestClusterSched, testCleanDyingPidByGroup3)
{
    ClusterSched scheduler;
    scheduler.groupMap_ = GroupMap{{"test_0_0_1", VmGroup{
                                                      .domainKey = uuid01 + "_0",
                                                      .id = uuid01 + "_0_0_0",
                                                      .clusterId = 0,
                                                      .layerId = 0,
                                                      .start = 0,
                                                      .nrCpus = 3,
                                                      .usedBitmap = DynamicBitset(CpuHelper::CLUSTER_CPU_NUM, false),
                                                      .entityPids = {266987, 266988, 266989},
                                                  }}};
    scheduler.numaClusterMap_ = NumaClusterMap{{0,
                                                {{0, Cluster{.id = 0,
                                                             .numaId = 0,
                                                             .cpuSet = {0, 1, 2, 3, 4, 5, 6, 7},
                                                             .clusterLayers = {ClusterLayer{
                                                                 .idle = 8,
                                                                 .total = 8,
                                                                 .usedBitmap = DynamicBitset(8),
                                                                 .groups = {},
                                                             }}}}}}};
    scheduler.entityMap_ = EntityMap{
        {266987,
         VmEntity{
             .pid = 266987,
             .cpuIdx = 0,
         }},
        {266988,
         VmEntity{
             .pid = 266987,
             .cpuIdx = 1,
         }},
    };
    VmDomain vmDomain;
    int tgidNum = 4567;
    vmDomain.uuid = "a1b2c3d4-e5f6-7890-abcd-ef1234567890";
    vmDomain.name = "ProductionVM-01";
    vmDomain.numaId = 0;
    vmDomain.tgid = tgidNum;
    vmDomain.pidVcpuMap = {{8001, 0}, {8002, 1}, {8003, 2}};
    vmDomain.groups = {"test_0_0_1"};
    vmDomain.entityPids = {4567, 4568, 4569};
    vmDomain.isReScheded = false;
    scheduler.CleanDyingPidByGroup(vmDomain);
}

TEST_F(TestClusterSched, testAllocGroupFromCluster)
{
    ClusterSched scheduler;
    VmDomain vmDomain;
    int numaIdNum = 2;
    int tgidNum = 4567;
    vmDomain.uuid = "a1b2c3d4-e5f6-7890-abcd-ef1234567890";
    vmDomain.name = "ProductionVM-01";
    vmDomain.numaId = numaIdNum;
    vmDomain.tgid = tgidNum;
    vmDomain.pidVcpuMap = {{8001, 0}, {8002, 1}, {8003, 2}};
    vmDomain.groups = {"production", "highperformance", "webcluster"};
    vmDomain.entityPids = {4567, 4568, 4569};
    vmDomain.isReScheded = false;

    Cluster cluster1;
    cluster1.id = 1;
    int clusterLayersSize = 3;
    int clusterLayersIdle = 5;
    int clusterLayersTotal = 10;
    cluster1.clusterLayers.resize(clusterLayersSize);
    cluster1.clusterLayers[0].idle = clusterLayersIdle;
    cluster1.clusterLayers[0].total = clusterLayersTotal;
    uint8_t layerId = 0;
    int bitSetTotal = 8;
    DynamicBitset bitset(bitSetTotal, false);

    scheduler.AllocGroupFromCluster(0, layerId, vmDomain, cluster1, bitset);
    EXPECT_TRUE(ClusterSched::GetInstance().groupMap_["production"].entityPids.empty());
    EXPECT_TRUE(ClusterSched::GetInstance().groupMap_["webcluster"].entityPids.empty());
}

TEST_F(TestClusterSched, testAllocGroupFromCluster2)
{
    ClusterSched scheduler;
    VmDomain vmDomain;
    int numaIdNum = 2;
    int tgidNum = 4567;
    vmDomain.uuid = "a1b2c3d4-e5f6-7890-abcd-ef1234567890";
    vmDomain.name = "ProductionVM-01";
    vmDomain.numaId = numaIdNum;
    vmDomain.tgid = tgidNum;
    vmDomain.pidVcpuMap = {{8001, 0}, {8002, 1}, {8003, 2}};
    vmDomain.groups = {"production", "highperformance", "webcluster"};
    vmDomain.entityPids = {4567, 4568, 4569};
    vmDomain.isReScheded = false;
    int cluserLayersSize = 3;
    int cluserLayersIdle = 5;
    int cluserLayersTotal = 10;
    int bitsetTotal = 8;
    Cluster cluster1;
    cluster1.id = 1;
    cluster1.clusterLayers.resize(cluserLayersSize);
    cluster1.clusterLayers[0].idle = cluserLayersIdle;
    cluster1.clusterLayers[0].total = cluserLayersTotal;
    uint8_t layerId = 0;

    DynamicBitset bitset(bitsetTotal, false);
    MOCKER(Bitset::FindFirstIdlePos).stubs().will(returnValue(int16_t(0)));
    MOCKER(ClusterSched::AddGroupToClusterLayer).stubs();
    MOCKER(ClusterSched::AddGroupToDomain).stubs();
    uint16_t res = scheduler.AllocGroupFromCluster(0, layerId, vmDomain, cluster1, bitset);
    EXPECT_EQ(res, 0);
    MOCKER(isIntInvalid).reset();
    MOCKER(ClusterSched::AddGroupToClusterLayer).reset();
    MOCKER(ClusterSched::AddGroupToDomain).reset();
}

TEST_F(TestClusterSched, testAddGroupToClusterLayer)
{
    ClusterSched scheduler;
    VmGroup vmGroup;
    ClusterLayer clusterLayer;
    scheduler.AddGroupToClusterLayer(vmGroup, clusterLayer);
}

TEST_F(TestClusterSched, testDelGroupFromClusterLayer)
{
    ClusterSched scheduler;
    VmGroup vmGroup;
    ClusterLayer clusterLayer;
    scheduler.DelGroupFromClusterLayer(vmGroup, clusterLayer);
}

TEST_F(TestClusterSched, testAddGroupToDomain)
{
    ClusterSched scheduler;
    VmGroup vmGroup;
    VmDomain vmDomain;
    scheduler.AddGroupToDomain(vmGroup, vmDomain);
}

TEST_F(TestClusterSched, testDelGroupFromDomain)
{
    ClusterSched scheduler;
    VmGroup vmGroup;
    VmDomain vmDomain;
    scheduler.DelGroupFromDomain(vmGroup, vmDomain);
}

TEST_F(TestClusterSched, testCompactionGroupFromLastLayer)
{
    ClusterSched scheduler;
    int cpuNum = 128;
    MOCKER(CpuHelper::GetClusterCpuSet).stubs().will(returnValue(cluster0CpuList));
    MOCKER(CpuHelper::GetMaxCpuNum).stubs().will(returnValue(cpuNum));
    MOCKER_CPP(&CpuHelper::GenCpuTopology, CpuTopologyMap(CpuHelper::*)())
        .stubs()
        .will(returnValue(defaultCpuTopologyMap));
    scheduler.overProvision_ = {{0, 1}, {1, 1}, {2, 1}, {3, 1}};
    Cluster cluster1;
    int clusterLayersSize = 3;
    int clusterLayersIdle = 5;
    int clusterLayersTotal = 10;
    cluster1.id = 1;
    cluster1.clusterLayers.resize(clusterLayersSize);
    cluster1.clusterLayers[0].idle = clusterLayersIdle;
    cluster1.clusterLayers[0].total = clusterLayersTotal;
    uint8_t layerId = 0;
    scheduler.CompactionGroupFromLastLayer(cluster1, layerId);
}

TEST_F(TestClusterSched, testCompactionGroupFromLastLayer2)
{
    ClusterSched scheduler;
    scheduler.numaClusterMap_ =
        NumaClusterMap{{1,
                        {{0, Cluster{.id = 0,
                                     .numaId = 0,
                                     .cpuSet = {0, 1, 2, 3, 4, 5, 6, 7},
                                     .clusterLayers = {ClusterLayer{
                                         .idle = 8,
                                         .total = 9,
                                         .usedBitmap = DynamicBitset(8),
                                         .groups = {"a1b2c3d4-e5f6-7890-abcd-ef1234567890_0_0_0"},
                                     }}}}}}};
    scheduler.overProvision_ = {{0, 1}, {1, 1}, {2, 1}, {3, 1}};
    Cluster cluster1;
    int clusterLayersSize = 3;
    int clusterLayersIdle = 5;
    int clusterLayersTotal = 10;
    cluster1.id = 0;
    cluster1.clusterLayers.resize(clusterLayersSize);
    cluster1.clusterLayers[0].idle = clusterLayersIdle;
    cluster1.clusterLayers[0].total = clusterLayersTotal;
    uint8_t layerId = 0;
    scheduler.CompactionGroupFromLastLayer(cluster1, layerId);
}

TEST_F(TestClusterSched, testCompactionGroupFromLastLayer3)
{
    ClusterSched scheduler;
    MOCKER(Bitset::FindFirstIdlePos).stubs().will(returnValue(0)).then(returnValue(0));
    MOCKER(isIntInvalid).stubs().will(returnValue(false)).then(returnValue(false));
    scheduler.numaClusterMap_ =
        NumaClusterMap{{1,
                        {{0, Cluster{.id = 0,
                                     .numaId = 0,
                                     .cpuSet = {0, 1, 2, 3, 4, 5, 6, 7},
                                     .clusterLayers = {ClusterLayer{
                                         .idle = 8,
                                         .total = 9,
                                         .usedBitmap = DynamicBitset(8),
                                         .groups = {"a1b2c3d4-e5f6-7890-abcd-ef1234567890_0_0_0"},
                                     }}}}}}};
    scheduler.overProvision_ = {{0, 1}, {1, 1}, {2, 1}, {3, 1}};
    Cluster cluster1;
    int clusterLayersSize = 3;
    int clusterLayersIdle = 5;
    int clusterLayersTotal = 10;
    cluster1.id = 0;
    cluster1.clusterLayers.resize(clusterLayersSize);
    cluster1.clusterLayers[0].idle = clusterLayersIdle;
    cluster1.clusterLayers[0].total = clusterLayersTotal;
    uint8_t layerId = 0;
    scheduler.CompactionGroupFromLastLayer(cluster1, layerId);
    MOCKER(&ClusterSched::GroupEntityMigrate).stubs().will(returnValue(VAS_ERROR));
    scheduler.CompactionGroupFromLastLayer(cluster1, layerId);
}

TEST_F(TestClusterSched, testCompactionClusterOneLayer)
{
    ClusterSched scheduler;
    std::map<uint16_t, Cluster> clusterMap;
    int clusterLayersSize = 3;
    int clusterLayersIdle = 5;
    int clusterLayersTotal = 10;
    Cluster cluster1;
    int clusterId = 1;
    cluster1.id = clusterId;
    cluster1.clusterLayers.resize(clusterLayersSize);
    cluster1.clusterLayers[0].idle = clusterLayersIdle;
    cluster1.clusterLayers[0].total = clusterLayersTotal;

    Cluster cluster2;
    int clusterId1 = 2;
    int clusterLayersIdle1 = 0;
    int clusterLayersTotal1 = 8;
    cluster2.id = clusterId;
    cluster2.clusterLayers.resize(clusterLayersSize);
    cluster2.clusterLayers[0].idle = clusterLayersIdle1;
    cluster2.clusterLayers[0].total = clusterLayersTotal1;

    Cluster cluster3;
    int clusterLayersIdle2 = 6;
    int clusterLayersTotal2 = 6;
    cluster3.id = clusterLayersSize;
    cluster3.clusterLayers.resize(clusterLayersSize);
    cluster3.clusterLayers[0].idle = clusterLayersIdle2;
    cluster3.clusterLayers[0].total = clusterLayersTotal2;

    // Add the cluster to the mapping table
    clusterMap[NUMBER_ONE] = cluster1;
    clusterMap[NUMBER_TWO] = cluster2;
    clusterMap[NUMBER_THREE] = cluster3;
    scheduler.CompactionClusterOneLayer(clusterMap, 0);
}

TEST_F(TestClusterSched, testGroupEntityMigrate)
{
    ClusterSched scheduler;
    VmGroup vmGroup;
    Cluster cluster1;
    int clusterLayersSize = 3;
    int clusterLayersIdle = 5;
    int clusterLayersTotal = 10;
    cluster1.id = 1;
    cluster1.clusterLayers.resize(clusterLayersSize);
    cluster1.clusterLayers[0].idle = clusterLayersIdle;
    cluster1.clusterLayers[0].total = clusterLayersTotal;
    uint8_t layerId = 0;
    int16_t start = 0;
    auto ret = scheduler.GroupEntityMigrate(vmGroup, start, cluster1, layerId);
    EXPECT_EQ(ret, VAS_OK);
}

TEST_F(TestClusterSched, testGroupEntityMigrate2)
{
    ClusterSched scheduler;
    int clusterLayersSize = 3;
    int clusterLayersIdle = 5;
    int clusterLayersTotal = 10;
    ClusterSched::GetInstance().entityMap_ = defaultEntityMap;
    VmGroup vmGroup;
    vmGroup.domainKey = uuid01 + "_0";
    vmGroup.id = uuid01 + "_0_0_0";
    vmGroup.clusterId = 0;
    vmGroup.layerId = 0;
    vmGroup.start = 0;
    vmGroup.nrCpus = clusterLayersSize;
    vmGroup.usedBitmap = DynamicBitset(CpuHelper::CLUSTER_CPU_NUM, false);
    vmGroup.entityPids = {266989};

    Cluster cluster1;

    cluster1.id = 0;
    cluster1.clusterLayers.resize(clusterLayersSize);
    cluster1.clusterLayers[0].idle = clusterLayersIdle;
    cluster1.clusterLayers[0].total = clusterLayersTotal;
    uint8_t layerId = 0;
    int16_t start = 0;
    auto ret = scheduler.GroupEntityMigrate(vmGroup, start, cluster1, layerId);
    EXPECT_EQ(ret, VAS_ERROR);
}

TEST_F(TestClusterSched, testGroupEntityMigrate3)
{
    ClusterSched scheduler;
    int clusterLayersSize = 3;
    int clusterLayersIdle = 5;
    int clusterLayersTotal = 10;
    ClusterSched::GetInstance().entityMap_ = defaultEntityMap;
    VmGroup vmGroup;
    vmGroup.domainKey = uuid01 + "_0";
    vmGroup.id = uuid01 + "_0_0_0";
    vmGroup.clusterId = 0;
    vmGroup.layerId = 0;
    vmGroup.start = 0;
    vmGroup.nrCpus = clusterLayersSize;
    vmGroup.usedBitmap = DynamicBitset(CpuHelper::CLUSTER_CPU_NUM, false);
    vmGroup.entityPids = {111111};

    Cluster cluster1;
    cluster1.id = 0;
    cluster1.clusterLayers.resize(clusterLayersSize);
    cluster1.clusterLayers[0].idle = clusterLayersIdle;
    cluster1.clusterLayers[0].total = clusterLayersTotal;
    uint8_t layerId = 0;
    int16_t start = 0;
    auto ret = scheduler.GroupEntityMigrate(vmGroup, start, cluster1, layerId);
    EXPECT_EQ(ret, VAS_ERROR);
}

TEST_F(TestClusterSched, testOverProvisionUp)
{
    ClusterSched scheduler;
    VmDomain vmDomain;
    ClusterSched::GetInstance().numaClusterMap_ = defaultNumaClusterMap;
    scheduler.Unassign(vmDomain);
    scheduler.OverProvisionUp(0);
    EXPECT_EQ(ClusterSched::GetInstance().numaClusterMap_.size(), 1);
}

TEST_F(TestClusterSched, testCompactionGroupWithinCluster)
{
    ClusterSched scheduler;
    int clusterId = 0;
    int clusterLayersSize = 3;
    int clusterLayersIdle0 = 0;
    int clusterLayersTotal0 = 7;
    Cluster cluster1;
    cluster1.id = clusterId;
    cluster1.clusterLayers.resize(clusterLayersSize);
    cluster1.clusterLayers[0].idle = clusterLayersIdle0;
    cluster1.clusterLayers[0].total = clusterLayersTotal0;

    Cluster cluster2;
    int clusterId1 = 2;
    int clusterLayersIdle1 = 8;
    int clusterLayersTotal1 = 14;
    cluster2.id = clusterId1;
    cluster2.clusterLayers.resize(clusterLayersSize);
    cluster2.clusterLayers[0].idle = clusterLayersIdle1;
    cluster2.clusterLayers[0].total = clusterLayersTotal1;
    cluster2.clusterLayers[0].groups = {"a1b2c3d4-e5f6-7890-abcd-ef1234567890_0_0_0"};

    uint8_t layerId = 2;
    ClusterSched::GetInstance().groupMap_ = defaultGroupMap;
    uint16_t numaId = 0;
    std::map<uint16_t, Cluster> clusterMap;
    MOCKER(&ClusterSched::CompactionClusterOneLayer).stubs().will(returnValue(0));
    scheduler.CompactionCluster(numaId, clusterMap);
    MOCKER(Bitset::FindFirstIdlePos).stubs().will(returnValue(0));
    MOCKER(isVasRetFail).stubs().will(returnValue(VAS_ERROR));
    scheduler.CompactionGroupWithinCluster(cluster1, cluster2, layerId);
    EXPECT_FALSE(ClusterSched::GetInstance().groupMap_[uuid01 + "_0_0_0"].entityPids.empty());
}

// AI-04: a dirty groupId (recorded in layer.groups but missing from groupMap_) must be
// skipped with the iterator advanced; otherwise the loop never terminates. This case returns
// normally only when the defensive branch advances the iterator.
TEST_F(TestClusterSched, testCompactionGroupWithinClusterDirtyGroupId)
{
    ClusterSched scheduler;
    const std::string dirtyGroupId = "ghost_group_not_in_groupmap";
    Cluster cluster1;
    cluster1.id = 0;
    cluster1.numaId = 0;
    cluster1.cpuSet = {0, 1, 2, 3, 4, 5, 6, 7};
    cluster1.clusterLayers.resize(1);
    cluster1.clusterLayers[0].idle = 8;
    cluster1.clusterLayers[0].total = 8;
    cluster1.clusterLayers[0].usedBitmap = DynamicBitset(8);

    Cluster cluster2;
    cluster2.id = 2;
    cluster2.numaId = 0;
    cluster2.cpuSet = {8, 9, 10, 11, 12, 13, 14, 15};
    cluster2.clusterLayers.resize(1);
    cluster2.clusterLayers[0].idle = 5;
    cluster2.clusterLayers[0].total = 8;
    cluster2.clusterLayers[0].usedBitmap = DynamicBitset(8);
    cluster2.clusterLayers[0].groups = {dirtyGroupId, uuid01 + "_0_0_0"};

    // The dirty id has no group entry; the valid group finds no consecutive idle cpu.
    scheduler.groupMap_ = defaultGroupMap;
    scheduler.domainMap_ = defaultDomainMap;
    uint8_t layerId = 0;
    MOCKER(Bitset::FindFirstIdlePos).stubs().will(returnValue(int16_t(-1)));
    // Would hang forever if the defensive branch did not advance the iterator.
    EXPECT_NO_THROW(scheduler.CompactionGroupWithinCluster(cluster1, cluster2, layerId));
    // Both the dirty id and the valid group stay in the source layer, nothing is lost.
    EXPECT_EQ(cluster2.clusterLayers[0].groups.count(dirtyGroupId), 1U);
    EXPECT_EQ(cluster2.clusterLayers[0].groups.count(uuid01 + "_0_0_0"), 1U);
    EXPECT_NE(scheduler.groupMap_.find(uuid01 + "_0_0_0"), scheduler.groupMap_.end());
}

TEST_F(TestClusterSched, testSetVmCpuset)
{
    ClusterSched scheduler;
    int bitSetTotal = 8;
    DynamicBitset bitset(bitSetTotal, false);
    MOCKER(scheduler.GetVmCgroupPath).stubs().will(returnValue(0));
    auto ret = scheduler.SetVmCpuset("test", VmThreadType::VCPU_CPUSET, bitset, 0);
    EXPECT_EQ(ret, VAS_ERROR);
    ret = scheduler.SetVmCpuset("test", VmThreadType::VCPU_PREFERRED_CPU, bitset, 0);
    EXPECT_EQ(ret, VAS_ERROR);
}
} // namespace vas::ut::sched