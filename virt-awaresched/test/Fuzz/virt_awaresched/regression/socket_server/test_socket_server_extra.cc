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

#include "../../../../cli/socket_server/test_socket_server.h"

#include <filesystem>

#include <mockcpp/mockcpp.hpp>

#include "socket_server.h"
#include "vas_security_manager.h"

namespace vas::ut::common {
using namespace vas::common;

namespace {
void FuzzSocketServerCloseServer(SocketServer *) {}
}

TEST_F(TestSocketServer, testStartServerDeleteCapabilitiesFailure)
{
    MOCKER_CPP(&SocketServer::BindSocket, bool (SocketServer::*)()).stubs().will(returnValue(true));
    MOCKER(vas::security::VasSecurityManager::ModifyEffectiveCapabilities)
        .stubs()
        .will(returnValue(static_cast<unsigned int>(0)))
        .then(returnValue(static_cast<unsigned int>(-1)));
    SocketServer socketServer;
    EXPECT_FALSE(socketServer.StartServer());
}

TEST_F(TestSocketServer, testStartServerListenFailure)
{
    MOCKER_CPP(&SocketServer::BindSocket, bool (SocketServer::*)()).stubs().will(returnValue(true));
    MOCKER_CPP(&SocketServer::CloseServer, void (SocketServer::*)()).stubs().will(invoke(FuzzSocketServerCloseServer));
    MOCKER(vas::security::VasSecurityManager::ModifyEffectiveCapabilities)
        .stubs()
        .will(returnValue(static_cast<unsigned int>(0)));
    MOCKER(listen).stubs().will(returnValue(-1));
    SocketServer socketServer;
    EXPECT_FALSE(socketServer.StartServer());
}

TEST_F(TestSocketServer, testRebuildRundirChmodFailure)
{
    SocketServer socketServer{};
    const auto testPath = std::filesystem::current_path() / "test_socket_dir_chmod";
    std::filesystem::remove_all(testPath);
    MOCKER(chmod).stubs().will(returnValue(1));
    EXPECT_FALSE(socketServer.RebuildRundir(testPath));
    MOCKER(chmod).reset();
    std::filesystem::remove_all(testPath);
}

TEST_F(TestSocketServer, testSocketResponseToString)
{
    SocketResponse response{1, "ok"};
    EXPECT_EQ(response.ToString(), R"("retCode":1,"retMsg":"ok")");
}
} // namespace vas::ut::common
