/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 *
 * VSched is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *      http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include "test_stress_api.h"

#include <mockcpp/mokc.h>

#include <array>

#include "api.h"
#include "cluster_sched.h"
#include "cmd.h"
#include "cmd_serialize.h"
#include "common/stress_config.h"
#include "vasd_arg_parse.h"

using namespace vas::common;
using namespace vas::sched;
using namespace vas::security::test;

namespace vas::security::test {

static void RecordAndVerifyStress(uint64_t done)
{
    const auto &info = GetLastStressRunInfo();
    testing::Test::RecordProperty("stress_count", std::to_string(done));
    testing::Test::RecordProperty("stress_elapsed_ms", std::to_string(info.elapsedMs));
    EXPECT_TRUE(StressAcceptanceReached());
}

static void RecordAttackProfile(uint64_t interfaceCalls, const std::string &version, const std::string &mask)
{
    testing::Test::RecordProperty("interface_calls", std::to_string(interfaceCalls));
    testing::Test::RecordProperty("attack_profile_version", version);
    testing::Test::RecordProperty("attack_categories_mask", mask);
}

static std::map<std::string, std::string> MakeMapAttackCase(const std::string &key, const std::string &valid,
                                                            uint64_t iteration)
{
    switch (iteration & 7U) {
        case 0:
            return {{key, valid}};
        case 1:
            return {}; // empty
        case 2:
            return {{"unknown-key", "value"}}; // missing expected key
        case 3:
            return {{key, "invalid-value"}};
        case 4:
            return {{key, std::string(2048, 'A')}}; // overlong
        case 5:
            return {{key, std::string("\0tail", 5)}}; // embedded NUL
        case 6:
            return {{key, ""}}; // boundary
        default:
            return {{key, "random-" + std::to_string(iteration * 0x9E3779B97F4A7C15ULL)}};
    }
}

static const std::array<std::string, 8> &GetCommandAttackCorpus()
{
    static const std::array<std::string, 8> corpus = {
        "setconfig;sched-policy:affinity",
        "",
        "setconfig",
        "unknown;key:value",
        std::string(4096, 'A'),
        std::string("setconfig;sched-policy:affinity") + std::string("\0tail", 5),
        "setconfig;sched-policy:affinity;sched-policy:dynamicAffinity",
        ";:;::;unknown:value",
    };
    return corpus;
}

void TestStressApi::SetUp()
{
    Test::SetUp();
    Api::cmdMap.clear();
    Api::cmdMap[std::string(SET_CMD) + SET_CONFIG_TYPE] = Api::SetConfig;
    Api::cmdMap[std::string(QUERY_CMD) + QUERY_AFFINITY_TYPE] = Api::QueryCpuAffinityInfo;
    Api::cmdMap[std::string(OPTION_CMD) + REASSIGN_TYPE] = Api::ReAssign;
    schedPolicyBackup_ = VasdArgParse::schedPolicy;
    MOCKER(VasdArgParse::DeInit).stubs().will(returnValue(VAS_OK));
    MOCKER(VasdArgParse::Init).stubs().will(returnValue(VAS_OK));
    MOCKER(&ClusterSched::ReSchedVm).stubs().will(returnValue(VAS_ERROR_INVAL));
    MOCKER_CPP(&ClusterSched::GetAffinityInfo,
               void (ClusterSched::*)(const std::string &, std::unordered_map<std::string, VmAffinity> &))
        .stubs()
        .will(ignoreReturnValue());
    MOCKER_CPP(&ClusterSched::ReSetSchedPolicy, void (ClusterSched::*)()).stubs().will(ignoreReturnValue());
}

void TestStressApi::TearDown()
{
    VasdArgParse::schedPolicy = schedPolicyBackup_;
    Api::cmdMap.clear();
    MOCKER(VasdArgParse::DeInit).reset();
    MOCKER(VasdArgParse::Init).reset();
    MOCKER(&ClusterSched::ReSchedVm).reset();
    MOCKER_CPP(&ClusterSched::GetAffinityInfo,
               void (ClusterSched::*)(const std::string &, std::unordered_map<std::string, VmAffinity> &))
        .reset();
    MOCKER_CPP(&ClusterSched::ReSetSchedPolicy, void (ClusterSched::*)()).reset();
    Test::TearDown();
}

std::string TestStressApi::schedPolicyBackup_ = "affinity";

// S-API-01: SetConfig 循环 N 次（默认 3000 万）或 3 小时
TEST_F(TestStressApi, testStressSetConfig)
{
    VasdArgParse::schedPolicy = "affinity";
    uint64_t iteration = 0;
    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("SetConfig", [&iteration]() {
                        const uint64_t current = iteration++;
                        auto data = MakeMapAttackCase(SCHED_POLICY, SCHED_POLICY_DYNAMIC, current);
                        if ((current & 7U) == 0U) {
                            VasdArgParse::schedPolicy = SCHED_POLICY_STATIC;
                        }
                        std::string resStr;
                        const auto ret = Api::SetConfig(data, resStr);
                        EXPECT_TRUE(ret == VAS_OK || ret == VAS_ERROR);
                        EXPECT_FALSE(resStr.empty());
                    }););
    RecordAndVerifyStress(done);
    RecordAttackProfile(done, "api-map-v1", "0xdf");
}

// S-API-02: QueryCpuAffinityInfo 循环 N 次
TEST_F(TestStressApi, testStressQueryCpuAffinityInfo)
{
    uint64_t iteration = 0;
    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("QueryCpuAffinityInfo", [&iteration]() {
                        auto data = MakeMapAttackCase(AFFINITY_SCOPE, "all", iteration++);
                        std::string resStr;
                        const auto ret = Api::QueryCpuAffinityInfo(data, resStr);
                        EXPECT_TRUE(ret == VAS_OK || ret == VAS_ERROR);
                        EXPECT_FALSE(resStr.empty());
                    }););
    RecordAndVerifyStress(done);
    RecordAttackProfile(done, "api-map-v1", "0xdf");
}

// S-API-03: ReAssign 循环 N 次
TEST_F(TestStressApi, testStressReAssign)
{
    uint64_t iteration = 0;
    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("ReAssign", [&iteration]() {
                        auto data = MakeMapAttackCase(REASSIGN_SCOPE, "all", iteration++);
                        std::string resStr;
                        const auto ret = Api::ReAssign(data, resStr);
                        EXPECT_TRUE(ret == VAS_OK || ret == VAS_ERROR);
                        EXPECT_FALSE(resStr.empty());
                    }););
    RecordAndVerifyStress(done);
    RecordAttackProfile(done, "api-map-v1", "0xdf");
}

// S-SER-01: DeSerialize 循环 N 次，吞吐稳定
TEST_F(TestStressApi, testStressDeSerialize)
{
    // 预构造一组合法 cmdStr 避免每次重新生成
    const std::string cmd = "setconfig;sched-policy:dynamicAffinity";
    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("DeSerialize", [&cmd]() {
                        CmdOption opt;
                        const auto ret = CmdSerialize::DeSerialize(cmd, opt);
                        EXPECT_TRUE(ret == VAS_OK || ret == VAS_ERROR);
                    }););
    RecordAndVerifyStress(done);
}

// S-API-04: SocketMsgHandler 端到端循环 N 次（含 DeSerialize + cmdMap 分派）
TEST_F(TestStressApi, testStressSocketMsgHandler)
{
    const auto &corpus = GetCommandAttackCorpus();
    uint64_t iteration = 0;
    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("SocketMsgHandler", [&]() {
                        std::string resStr;
                        const auto ret = Api::SocketMsgHandler(corpus[iteration++ % corpus.size()], resStr);
                        EXPECT_TRUE(ret == VAS_OK || ret == VAS_ERROR);
                        EXPECT_FALSE(resStr.empty());
                    }););
    RecordAndVerifyStress(done);
    RecordAttackProfile(done, "api-command-v1", "0xff");
}

} // namespace vas::security::test
