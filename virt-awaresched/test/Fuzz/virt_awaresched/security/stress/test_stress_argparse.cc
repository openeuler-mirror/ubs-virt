/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 *
 * VSched is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *      http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include "test_stress_argparse.h"

#include <mockcpp/mockcpp.hpp>

#include <iostream>
#include <streambuf>

#include "cluster_sched.h"
#include "cmd.h"
#include "common/stress_config.h"
#include "common/vasctl_test_exports.h"
#include "cpu_helper.h"
#include "libvirt_helper.h"
#include "logger.h"
#include "socket_client.h"
#include "vas_cli_parse.h"
#include "vas_cli_process_ctl.h"
#include "vas_cli_reg_builder.h"
#include "vas_cli_res_echo.h"
#include "vasctl_arg_parse.h"
#include "vasd_arg_parse.h"

using namespace vas::cli::framework;
using namespace vas::cli::reg;
using namespace vas::common;
using namespace vas::sched;
using namespace vas::sched::acquire;
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
            return {};
        case 2:
            return {{"unknown-key", "value"}};
        case 3:
            return {{key, "invalid-value"}};
        case 4:
            return {{key, std::string(2048, 'A')}};
        case 5:
            return {{key, std::string("\0tail", 5)}};
        case 6:
            return {{key, ""}};
        default:
            return {{key, "random-" + std::to_string(iteration * 0x9E3779B97F4A7C15ULL)}};
    }
}

// 静音 std::cout/std::cerr：CliSetServerConfFunc 每轮会打印多行，3000 万次会产生海量日志拖慢执行。
class NullBuf : public std::streambuf {
protected:
    int overflow(int c) override
    {
        return c;
    }
    std::streamsize xsputn(const char *, std::streamsize n) override
    {
        return n;
    }
};
NullBuf g_nullBuf;
struct StreamMuter {
    std::streambuf *coutBuf = nullptr;
    std::streambuf *cerrBuf = nullptr;
    StreamMuter()
    {
        coutBuf = std::cout.rdbuf(&g_nullBuf);
        cerrBuf = std::cerr.rdbuf(&g_nullBuf);
    }
    ~StreamMuter()
    {
        std::cout.rdbuf(coutBuf);
        std::cerr.rdbuf(cerrBuf);
    }
};

static bool StubSendSingleMessage(const std::string &)
{
    return true;
}

static void ResetVasdArgs()
{
    vas::sched::VasdArgParse::smt = true;
    vas::sched::VasdArgParse::schedPolicy = "affinity";
    vas::sched::VasdArgParse::dynamicAffinityUtilThresh = 85;
    vas::sched::VasdArgParse::skippedCPUSet = "";
    vas::sched::VasdArgParse::rangeAffinity = true;
}

void TestStressArgParse::SetUp()
{
    Test::SetUp();
    ResetVasdArgs();
}

void TestStressArgParse::TearDown()
{
    ResetVasdArgs();
    GlobalMockObject::verify();
    Test::TearDown();
}

// E7: vasctl 命令回调 CliSetConfFunc 循环 N 次（随机入参，mock SendSingleMessage 避免真实 socket）
TEST_F(TestStressArgParse, testStressCliSetConfFunc)
{
    MOCKER(SendSingleMessage).stubs().will(invoke(StubSendSingleMessage));

    uint64_t iteration = 0;
    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("CliSetConfFunc", [&iteration]() {
                        auto data = MakeMapAttackCase(SCHED_POLICY, SCHED_POLICY_DYNAMIC, iteration++);
                        (void)CliSetConfFunc(data);
                    }););
    RecordAndVerifyStress(done);
    RecordAttackProfile(done, "cli-map-v1", "0xdf");
    MOCKER(SendSingleMessage).reset();
}

// E7: vasctl 命令回调 CliQueryAffinityFunc 循环 N 次
TEST_F(TestStressArgParse, testStressCliQueryAffinityFunc)
{
    MOCKER(SendSingleMessage).stubs().will(invoke(StubSendSingleMessage));

    uint64_t iteration = 0;
    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("CliQueryAffinityFunc", [&iteration]() {
                        auto data = MakeMapAttackCase(AFFINITY_SCOPE, "all", iteration++);
                        (void)CliQueryAffinityFunc(data);
                    }););
    RecordAndVerifyStress(done);
    RecordAttackProfile(done, "cli-map-v1", "0xdf");
    MOCKER(SendSingleMessage).reset();
}

// E7: vasctl 命令回调 CliOptReassignFunc 循环 N 次
TEST_F(TestStressArgParse, testStressCliOptReassignFunc)
{
    MOCKER(SendSingleMessage).stubs().will(invoke(StubSendSingleMessage));

    uint64_t iteration = 0;
    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("CliOptReassignFunc", [&iteration]() {
                        auto data = MakeMapAttackCase(REASSIGN_SCOPE, "all", iteration++);
                        (void)CliOptReassignFunc(data);
                    }););
    RecordAndVerifyStress(done);
    RecordAttackProfile(done, "cli-map-v1", "0xdf");
    MOCKER(SendSingleMessage).reset();
}

// E7: vasctl 命令回调 CliOptRecoverFunc 循环 N 次（mock 重依赖：Logger/CpuHelper/LibvirtHelper/ClusterSched）
TEST_F(TestStressArgParse, testStressCliOptRecoverFunc)
{
    StreamMuter muter;
    MOCKER_CPP(&Logger::Init, VasRet(Logger::*)(const std::string &, const std::string &, const size_t &, const int &,
                                                const OutputType &))
        .stubs()
        .will(returnValue(VAS_OK));
    MOCKER(CpuHelper::Init).stubs().will(returnValue(VAS_OK));
    MOCKER_CPP(&CpuHelper::GenCpuTopology, CpuTopologyMap(CpuHelper::*)(void))
        .stubs()
        .will(returnValue(CpuTopologyMap{}));
    MOCKER_CPP(&LibvirtHelper::Init, VasRet(LibvirtHelper::*)()).stubs().will(returnValue(VAS_OK));
    MOCKER_CPP(&LibvirtHelper::GetVmInfoList, VasRet(LibvirtHelper::*)(VmInfoMap &)).stubs().will(returnValue(VAS_OK));
    MOCKER_CPP(&LibvirtHelper::DeInit, void (LibvirtHelper::*)()).stubs().will(ignoreReturnValue());
    MOCKER_CPP(&ClusterSched::RecoverVmVcpu, void (ClusterSched::*)(const VmInfoMap &))
        .stubs()
        .will(ignoreReturnValue());

    uint64_t iteration = 0;
    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("CliOptRecoverFunc", [&iteration]() {
                        auto data = MakeMapAttackCase(RECOVER_VM, "all", iteration++);
                        (void)CliOptRecoverFunc(data);
                    }););
    RecordAndVerifyStress(done);
    RecordAttackProfile(done, "cli-map-v1", "0xdf");
    MOCKER_CPP(&Logger::Init, VasRet(Logger::*)(const std::string &, const std::string &, const size_t &, const int &,
                                                const OutputType &))
        .reset();
    MOCKER(CpuHelper::Init).reset();
    MOCKER_CPP(&CpuHelper::GenCpuTopology, CpuTopologyMap(CpuHelper::*)(void)).reset();
    MOCKER_CPP(&LibvirtHelper::Init, VasRet(LibvirtHelper::*)()).reset();
    MOCKER_CPP(&LibvirtHelper::GetVmInfoList, VasRet(LibvirtHelper::*)(VmInfoMap &)).reset();
    MOCKER_CPP(&LibvirtHelper::DeInit, void (LibvirtHelper::*)()).reset();
    MOCKER_CPP(&ClusterSched::RecoverVmVcpu, void (ClusterSched::*)(const VmInfoMap &)).reset();
}

// E8: vas_daemon 启动参数 CliSetServerConfFunc 循环 N 次（随机入参，覆盖 stoi 异常分支/布尔解析）
TEST_F(TestStressArgParse, testStressCliSetServerConfFunc)
{
    StreamMuter muter;

    uint64_t iteration = 0;
    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("CliSetServerConfFunc", [&iteration]() {
                        auto data = MakeMapAttackCase("dynamic-util-thresh", "85", iteration++);
                        (void)CliSetServerConfFunc(data);
                    }););
    RecordAndVerifyStress(done);
    RecordAttackProfile(done, "server-cli-map-v1", "0xdf");
}

// E8: vas_daemon 启动参数注册 RegisterServerModuleSDK 循环 N 次（命令注册路径）
TEST_F(TestStressArgParse, testStressRegisterServerModuleSDK)
{
    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("RegisterServerModuleSDK", []() {
                        auto &parser = vas::cli::framework::VasCliParse::GetInstance();
                        parser.Reset();
                        RegisterServerModuleSDK();
                        const auto &info = parser.GetSdkCmdInfo();
                        EXPECT_FALSE(info.empty());
                    }););
    RecordAndVerifyStress(done);
}

// E6 执行层：argv 经过 MainExecuteProcess；ExecuteCommand 被替换，边界止于业务回调。
TEST_F(TestStressArgParse, testStressMainExecuteProcess)
{
    StreamMuter muter;
    MOCKER_CPP(&VasCliResEcho::ExecuteCommand, VasRet(VasCliResEcho::*)()).stubs().will(returnValue(VAS_OK));

    uint64_t iteration = 0;
    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("MainExecuteProcess", [&iteration]() {
                        auto &parser = VasCliParse::GetInstance();
                        parser.Reset();
                        RegisterCliModuleSDK();

                        char program[] = "vasctl";
                        char set[] = "set";
                        char config[] = "config";
                        char policy[] = "--sched-policy";
                        char valid[] = "affinity";
                        char unknown[] = "--unknown";
                        char missing[] = "";
                        char help[] = "--help";
                        char empty[] = "";
                        char overlong[2049];
                        std::fill(std::begin(overlong), std::end(overlong), 'A');
                        overlong[2048] = '\0';
                        char nulValue[] = "affinity\0tail";
                        char boundary[] = "-";
                        char random[] = "%%%";

                        char *validArgs[] = {program, set, config, policy, valid};
                        char *unknownArgs[] = {program, set, config, unknown, valid};
                        char *missingArgs[] = {program, set, config, policy, missing};
                        char *helpArgs[] = {program, help};
                        char *emptyArgs[] = {program, empty};
                        char *overlongArgs[] = {program, set, config, policy, overlong};
                        char *nulArgs[] = {program, set, config, policy, nulValue};
                        char *boundaryArgs[] = {program, set, config, policy, boundary};
                        char *randomArgs[] = {program, random};
                        VasRet ret = VAS_ERROR;
                        switch (iteration++ & 7U) {
                            case 0:
                                ret = VasCliProcessCtl::MainExecuteProcess(5, validArgs);
                                break;
                            case 1:
                                ret = VasCliProcessCtl::MainExecuteProcess(5, unknownArgs);
                                break;
                            case 2:
                                ret = VasCliProcessCtl::MainExecuteProcess(5, missingArgs);
                                break;
                            case 3:
                                ret = VasCliProcessCtl::MainExecuteProcess(5, boundaryArgs);
                                break;
                            case 4:
                                ret = VasCliProcessCtl::MainExecuteProcess(2, emptyArgs);
                                break;
                            case 5:
                                ret = VasCliProcessCtl::MainExecuteProcess(5, overlongArgs);
                                break;
                            case 6:
                                ret = VasCliProcessCtl::MainExecuteProcess(5, nulArgs);
                                break;
                            default:
                                ret = VasCliProcessCtl::MainExecuteProcess(2, randomArgs);
                                break;
                        }
                        EXPECT_TRUE(ret == VAS_OK || ret == VAS_ERROR || ret == VAS_ERROR_CMD);
                    }););
    RecordAndVerifyStress(done);
    RecordAttackProfile(done, "cli-argv-v1", "0xdf");
    MOCKER_CPP(&VasCliResEcho::ExecuteCommand, VasRet(VasCliResEcho::*)()).reset();
}
} // namespace vas::security::test
