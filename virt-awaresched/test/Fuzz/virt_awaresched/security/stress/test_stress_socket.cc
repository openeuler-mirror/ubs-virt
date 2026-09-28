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

#include "test_stress_socket.h"

#include <securec.h>

#include <mockcpp/mockcpp.hpp>

#include <array>
#include <cerrno>
#include <filesystem>
#include <iostream>
#include <streambuf>
#include <string>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "api.h"
#include "common/stress_config.h"
#include "socket_client.h"
#include "socket_server.h"
#include "vas_security_manager.h"

using namespace vas::common;
using namespace vas::security::test;

namespace vas::security::test {

class SocketNullBuf final : public std::streambuf {
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

class SocketStreamMuter {
public:
    SocketStreamMuter() : coutBuf_(std::cout.rdbuf(&nullBuf_)), cerrBuf_(std::cerr.rdbuf(&nullBuf_)) {}
    ~SocketStreamMuter()
    {
        std::cout.rdbuf(coutBuf_);
        std::cerr.rdbuf(cerrBuf_);
    }

private:
    SocketNullBuf nullBuf_;
    std::streambuf *coutBuf_;
    std::streambuf *cerrBuf_;
};

static void RecordAndVerifyStress(uint64_t done)
{
    const auto &info = GetLastStressRunInfo();
    testing::Test::RecordProperty("stress_count", std::to_string(done));
    testing::Test::RecordProperty("stress_elapsed_ms", std::to_string(info.elapsedMs));
    EXPECT_TRUE(StressAcceptanceReached());
}

// mock 收发的辅助桩
static ssize_t StubRecvReturn1(int, void *buf, size_t, int)
{
    *static_cast<char *>(buf) = 'a';
    return 1;
}
static ssize_t StubRecvReturn0(int, void *, size_t, int)
{
    return 0;
}
static ssize_t StubSendOk(int, const void *, size_t n, int)
{
    return static_cast<ssize_t>(n);
}
static int StubSocketFd(int, int, int)
{
    return 1;
}
static int StubAcceptReturn1(int, struct sockaddr *, socklen_t *)
{
    return 1;
}
static int StubConnectOk(int, const struct sockaddr *, socklen_t)
{
    return 0;
}
static uint64_t g_connectAttackIteration = 0;
static int StubSocketAttack(int, int, int)
{
    return (g_connectAttackIteration & 7U) == 1U ? -1 : 1;
}
static int StubConnectAttack(int, const struct sockaddr *, socklen_t)
{
    return (g_connectAttackIteration & 7U) == 2U ? -1 : 0;
}
static errno_t StubMemcpyAttack(void *, size_t, const void *, size_t)
{
    return (g_connectAttackIteration & 7U) == 3U ? EINVAL : EOK;
}
static uint64_t g_receiveAttackIteration = 0;
static ssize_t StubRecvAttack(int, void *buf, size_t, int)
{
    switch (g_receiveAttackIteration++ & 3U) {
        case 0:
            *static_cast<char *>(buf) = 'a';
            return 1;
        case 1:
            return 0;
        case 2:
            errno = EINTR;
            return -1;
        default:
            static_cast<char *>(buf)[0] = 'a';
            static_cast<char *>(buf)[1] = 'b';
            return 2;
    }
}

static const std::array<std::string, 8> &GetUdsAttackCorpus()
{
    static const std::array<std::string, 8> corpus = {
        "setconfig;sched-policy:not-a-policy",                                     // invalid value
        "",                                                                        // empty
        "setconfig",                                                               // missing parameter
        "garbage;missing-colon",                                                   // malformed
        std::string(4096, 'A'),                                                    // overlong/truncated frame
        std::string("setconfig;sched-policy:affinity") + std::string("\0tail", 5), // embedded NUL
        "setconfig;sched-policy:affinity;sched-policy:dynamicAffinity",            // duplicate key
        ";:;::;unknown:value",                                                     // boundary tokens
    };
    return corpus;
}

static bool SendAll(int fd, const std::string &message)
{
    size_t sent = 0;
    while (sent < message.size()) {
        const ssize_t result = send(fd, message.data() + sent, message.size() - sent, 0);
        if (result <= 0) {
            return false;
        }
        sent += static_cast<size_t>(result);
    }
    return true;
}

void TestStressSocket::SetUp()
{
    Test::SetUp();
}

void TestStressSocket::TearDown()
{
    GlobalMockObject::verify();
    Test::TearDown();
}

// S-SOCK-01: SocketClient.SendMessage 循环 N 次，无 fd 泄漏
TEST_F(TestStressSocket, testStressClientSendMessage)
{
    MOCKER_CPP(&SocketClient::CloseConnection, void (SocketClient::*)()).stubs().will(ignoreReturnValue());
    MOCKER(close).stubs().will(returnValue(0));
    SocketClient client;
    client.isConnected = true;
    MOCKER(send).stubs().will(invoke(StubSendOk));

    const auto &corpus = GetUdsAttackCorpus();
    uint64_t iteration = 0;
    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("ClientSendMessage", [&client, &corpus, &iteration]() {
                        EXPECT_TRUE(client.SendMessage(corpus[iteration++ % corpus.size()]));
                    }););
    RecordAndVerifyStress(done);
    testing::Test::RecordProperty("interface_calls", std::to_string(done));
    testing::Test::RecordProperty("attack_profile_version", "socket-send-v1");
    testing::Test::RecordProperty("attack_categories_mask", "0xff");
    MOCKER(send).reset();
}

// S-SOCK-00: SocketClient.ConnectToServer 独立循环（L2：mock syscall，不是实际 UDS 链路）
TEST_F(TestStressSocket, testStressClientConnectToServer)
{
    SocketStreamMuter muter;
    MOCKER(close).stubs().will(returnValue(0));
    MOCKER(socket).stubs().will(invoke(StubSocketAttack));
    MOCKER(connect).stubs().will(invoke(StubConnectAttack));
    MOCKER(memcpy_s).stubs().will(invoke(StubMemcpyAttack));

    g_connectAttackIteration = 0;
    for (uint64_t mode = 0; mode < 4; ++mode) {
        g_connectAttackIteration = mode;
        SocketClient client;
        const bool connected = client.ConnectToServer();
        EXPECT_EQ(connected, mode == 0U);
        client.CloseConnection();
    }
    g_connectAttackIteration = 4;
    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("ClientConnectToServer", []() {
                        SocketClient client;
                        EXPECT_TRUE(client.ConnectToServer());
                        client.CloseConnection();
                    }););
    RecordAndVerifyStress(done);
    testing::Test::RecordProperty("interface_calls", std::to_string(done));
    testing::Test::RecordProperty("attack_profile_version", "socket-connect-fault-v1");
    testing::Test::RecordProperty("attack_categories_mask", "0x0f");
    testing::Test::RecordProperty("attack_preflight_calls", "4");
    MOCKER(socket).reset();
    MOCKER(connect).reset();
    MOCKER(memcpy_s).reset();
}

// S-SOCK-01B: SocketClient.ReceiveMessage 独立循环（与服务端 ReceiveMessage 分开计数）
TEST_F(TestStressSocket, testStressClientReceiveMessage)
{
    SocketStreamMuter muter;
    MOCKER(close).stubs().will(returnValue(0));
    MOCKER(recv).stubs().will(invoke(StubRecvAttack));
    SocketClient client;

    g_receiveAttackIteration = 0;
    for (uint64_t mode = 0; mode < 4; ++mode) {
        g_receiveAttackIteration = mode;
        client.clientSocket = 1;
        client.isConnected = true;
        const auto message = client.ReceiveMessage();
        EXPECT_TRUE(message.empty() || message == "a" || message == "ab");
    }
    MOCKER(recv).reset();
    MOCKER(recv).stubs().will(invoke(StubRecvReturn1));
    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("ClientReceiveMessage", [&client]() {
                        client.clientSocket = 1;
                        client.isConnected = true;
                        EXPECT_EQ(client.ReceiveMessage(), "a");
                    }););
    RecordAndVerifyStress(done);
    testing::Test::RecordProperty("interface_calls", std::to_string(done));
    testing::Test::RecordProperty("attack_profile_version", "socket-receive-fault-v1");
    testing::Test::RecordProperty("attack_categories_mask", "0x0f");
    testing::Test::RecordProperty("attack_preflight_calls", "4");
    MOCKER(recv).reset();
}

// S-SOCK-02: SocketServer.ReceiveMessage 循环 N 次（mock recv 返回 1 字节）
TEST_F(TestStressSocket, testStressServerReceiveMessage)
{
    MOCKER_CPP(&SocketServer::CloseServer, void (SocketServer::*)()).stubs().will(ignoreReturnValue());
    MOCKER(close).stubs().will(returnValue(0));
    SocketServer server;
    MOCKER(recv).stubs().will(invoke(StubRecvAttack));

    g_receiveAttackIteration = 0;
    for (uint64_t mode = 0; mode < 4; ++mode) {
        g_receiveAttackIteration = mode;
        const auto msg = server.ReceiveMessage();
        EXPECT_TRUE(msg.empty() || msg == "a" || msg == "ab");
    }
    MOCKER(recv).reset();
    MOCKER(recv).stubs().will(invoke(StubRecvReturn1));
    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("ServerReceiveMessage", [&server]() {
                        const auto msg = server.ReceiveMessage();
                        EXPECT_EQ(msg, "a");
                    }););
    RecordAndVerifyStress(done);
    testing::Test::RecordProperty("interface_calls", std::to_string(done));
    testing::Test::RecordProperty("attack_profile_version", "socket-server-receive-v1");
    testing::Test::RecordProperty("attack_categories_mask", "0x0f");
    testing::Test::RecordProperty("attack_preflight_calls", "4");
    MOCKER(recv).reset();
}

// S-SOCK-03: SocketServer.AcceptClient + SendMessage 循环
TEST_F(TestStressSocket, testStressServerAcceptAndSend)
{
    MOCKER_CPP(&SocketServer::CloseServer, void (SocketServer::*)()).stubs().will(ignoreReturnValue());
    MOCKER(close).stubs().will(returnValue(0));
    SocketServer server;
    MOCKER(accept).stubs().will(invoke(StubAcceptReturn1));
    MOCKER(send).stubs().will(invoke(StubSendOk));

    const auto &corpus = GetUdsAttackCorpus();
    uint64_t iteration = 0;
    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("ServerAcceptAndSend", [&server, &corpus, &iteration]() {
                        EXPECT_TRUE(server.AcceptClient());
                        EXPECT_TRUE(server.SendMessage(corpus[iteration++ % corpus.size()]));
                    }););
    RecordAndVerifyStress(done);
    testing::Test::RecordProperty("interface_calls", std::to_string(done));
    testing::Test::RecordProperty("attack_profile_version", "socket-server-send-v1");
    testing::Test::RecordProperty("attack_categories_mask", "0xff");
    MOCKER(accept).reset();
    MOCKER(send).reset();
}

// S-SOCK-04: SocketServer.StartServer 路径循环（BindSocket/StartServer）
TEST_F(TestStressSocket, testStressServerStartServer)
{
    MOCKER_CPP(&SocketServer::CloseServer, void (SocketServer::*)()).stubs().will(ignoreReturnValue());
    MOCKER_CPP(&SocketServer::RebuildRundir, bool (SocketServer::*)(const fs::path &)).stubs().will(returnValue(true));
    MOCKER(close).stubs().will(returnValue(0));
    MOCKER(socket).stubs().will(invoke(StubSocketFd));
    MOCKER(listen).stubs().will(returnValue(0));
    MOCKER(bind).stubs().will(returnValue(0));
    MOCKER(chmod).stubs().will(returnValue(0));
    MOCKER(memcpy_s).stubs().will(returnValue(EOK));
    MOCKER(std::filesystem::remove).stubs().will(returnValue(true));
    MOCKER(vas::security::VasSecurityManager::ModifyEffectiveCapabilities).stubs().will(returnValue(VAS_OK));

    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("ServerStartServer", []() {
                        SocketServer server;
                        // 不强断言成功，只断言不崩溃/无异常/无 fd 泄漏
                        EXPECT_NO_THROW(server.StartServer());
                    }););
    RecordAndVerifyStress(done);
    MOCKER_CPP(&SocketServer::RebuildRundir, bool (SocketServer::*)(const fs::path &)).reset();
    MOCKER(socket).reset();
    MOCKER(listen).reset();
    MOCKER(bind).reset();
    MOCKER(chmod).reset();
    MOCKER(memcpy_s).reset();
    MOCKER(std::filesystem::remove).reset();
    MOCKER(vas::security::VasSecurityManager::ModifyEffectiveCapabilities).reset();
}

// E1 L3: real UDS transport through SocketServer and Api::SocketMsgHandler.
// No socket-family syscall is mocked in this test. The temporary endpoint is isolated under /tmp.
TEST_F(TestStressSocket, testStressRealUdsFullChain)
{
    const std::string socketPath = "/tmp/vas-security-" + std::to_string(getpid()) + ".sock";
    (void)unlink(socketPath.c_str());

    const int listenFd = socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_GE(listenFd, 0);

    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    ASSERT_LT(socketPath.size(), sizeof(address.sun_path));
    ASSERT_EQ(memcpy_s(address.sun_path, sizeof(address.sun_path), socketPath.data(), socketPath.size()), EOK);
    ASSERT_EQ(bind(listenFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)), 0);
    ASSERT_EQ(listen(listenFd, 3), 0);

    SocketServer server;
    server.serverFd = listenFd;
    const auto &corpus = GetUdsAttackCorpus();
    uint64_t iteration = 0;
    uint64_t interfaceCalls = 0;
    uint64_t done = 0;
    ASSERT_NO_THROW(done = RunStress("RealUdsFullChain", [&]() {
                        const int clientFd = socket(AF_UNIX, SOCK_STREAM, 0);
                        ASSERT_GE(clientFd, 0);
                        ASSERT_EQ(connect(clientFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)), 0);
                        ASSERT_TRUE(server.AcceptClient());

                        const std::string &request = corpus[iteration++ % corpus.size()];
                        ASSERT_TRUE(SendAll(clientFd, request));
                        (void)shutdown(clientFd, SHUT_WR);

                        const std::string received = server.ReceiveMessage();
                        std::string response;
                        const VasRet ret = vas::sched::Api::SocketMsgHandler(received, response);
                        EXPECT_TRUE(ret == VAS_OK || ret == VAS_ERROR);
                        EXPECT_FALSE(response.empty());
                        ASSERT_TRUE(server.SendMessage(response));

                        std::array<char, 4096> reply{};
                        ASSERT_GT(recv(clientFd, reply.data(), reply.size(), 0), 0);
                        (void)close(clientFd);
                        (void)close(server.clientSocket);
                        server.clientSocket = 0;
                        ++interfaceCalls;
                    }););

    RecordAndVerifyStress(done);
    RecordProperty("interface_calls", std::to_string(interfaceCalls));
    RecordProperty("attack_profile_version", "uds-v1");
    RecordProperty("attack_categories_mask", "0xff");
    RecordProperty("transport", "real_uds");
    RecordProperty("mock_socket_calls", "0");

    server.CloseServer();
    (void)unlink(socketPath.c_str());
}

} // namespace vas::security::test
