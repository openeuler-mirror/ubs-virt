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

#include "test_socket_server.h"

#include <mockcpp/mockcpp.hpp>
#include <securec.h>

#include "socket_server.h"
#include "vas_security_manager.h"

namespace vas::ut::common {
using namespace vas::common;
inline constexpr int NUMBER_NEGATIVE_ONE = -1;
inline constexpr int NUMBER_ONE = 1;
inline constexpr int NUMBER_TEN = 10;
void TestSocketServer::SetUp()
{
    Test::SetUp();
}

void TestSocketServer::TearDown()
{
    GlobalMockObject::verify();
    Test::TearDown();
}

void SocketServerCloseServer(SocketServer *_this) {}

TEST_F(TestSocketServer, testStartServer)
{
    MOCKER_CPP(&vas::common::SocketServer::CloseServer, void(vas::common::SocketServer::*)())
        .stubs()
        .will(invoke(SocketServerCloseServer));
    MOCKER(close).stubs().will(returnValue(NUMBER_ONE));
    MOCKER(socket).stubs().will(returnValue(NUMBER_NEGATIVE_ONE)).then(returnValue(NUMBER_ONE));
    MOCKER(vas::security::VasSecurityManager::ModifyEffectiveCapabilities).stubs()
        .will(returnValue(0)).then(returnValue(0));
    SocketServer socketServer;
    auto ret = socketServer.StartServer();
    EXPECT_FALSE(ret);
    MOCKER(bind).stubs().will(returnValue(NUMBER_NEGATIVE_ONE)).then(returnValue(NUMBER_ONE));
    ret = socketServer.StartServer();
    EXPECT_FALSE(ret);
    MOCKER(chmod).stubs().will(returnValue(NUMBER_NEGATIVE_ONE)).then(returnValue(NUMBER_ONE));
    ret = socketServer.StartServer();
    EXPECT_FALSE(ret);
    MOCKER(listen).stubs().will(returnValue(NUMBER_NEGATIVE_ONE)).then(returnValue(NUMBER_ONE));
    ret = socketServer.StartServer();
    EXPECT_FALSE(ret);
    ret = socketServer.StartServer();
    EXPECT_TRUE(ret);
}

TEST_F(TestSocketServer, testAcceptClient)
{
    MOCKER_CPP(&SocketServer::CloseServer, void(SocketServer::*)()).stubs().will(invoke(SocketServerCloseServer));
    MOCKER(close).stubs().will(returnValue(NUMBER_ONE));
    SocketServer socketServer{};
    MOCKER(accept).stubs().will(returnValue(NUMBER_NEGATIVE_ONE)).then(returnValue(NUMBER_ONE));
    auto ret = socketServer.AcceptClient();
    EXPECT_FALSE(ret);
    ret = socketServer.AcceptClient();
    EXPECT_TRUE(ret);
}

ssize_t ServerRecvMock(int fd, void *buf, size_t n, int flags)
{
    auto *buff = static_cast<char *>(buf);
    *buff = 'a';
    return NUMBER_ONE;
}

TEST_F(TestSocketServer, testReceiveMessage)
{
    MOCKER_CPP(&SocketServer::CloseServer, void(SocketServer::*)()).stubs().will(invoke(SocketServerCloseServer));
    MOCKER(close).stubs().will(returnValue(NUMBER_ONE));
    SocketServer socketServer{};
    MOCKER(recv).stubs().will(returnValue(static_cast<ssize_t>(NUMBER_NEGATIVE_ONE1))).then(invoke(ServerRecvMock));
    auto ret = socketServer.ReceiveMessage();
    EXPECT_EQ(ret, "");
    ret = socketServer.ReceiveMessage();
    EXPECT_EQ(ret, "a");
}

ssize_t ServerSendMock(int fd, const void *buf, size_t n, int flags)
{
    auto *buff = static_cast<const char *>(buf);
    if (*buff == 'a') {
        return NUMBER_ONE;
    }
    return NUMBER_TEN;
};

TEST_F(TestSocketServer, testSendMessage)
{
    MOCKER_CPP(&SocketServer::CloseServer, void(SocketServer::*)()).stubs().will(invoke(SocketServerCloseServer));
    MOCKER(close).stubs().will(returnValue(NUMBER_ONE));
    MOCKER(send).stubs().will(invoke(ServerSendMock));
    SocketServer socketServer;
    auto ret = socketServer.SendMessage("b");
    EXPECT_FALSE(ret);
    ret = socketServer.SendMessage("a");
    EXPECT_TRUE(ret);
}

TEST_F(TestSocketServer, testCloseServer)
{
    MOCKER(close).stubs().will(returnValue(NUMBER_ONE));
    SocketServer socketServer;
    socketServer.clientSocket = NUMBER_ONE;
    socketServer.serverFd = NUMBER_ONE;
    socketServer.CloseServer();
}
} // namespace vas::ut::common