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
#include "server.h"
#include "logger.h"
#include "virt_ipc_code.h"

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <atomic>
#include <chrono>

using namespace virt::ovs;
using namespace virt::ovs::msg;
namespace virt::ovs::ipc::server {
static void SetNonBlock(int fd)
{
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

Server::Server(const std::string &sockPath, size_t workers) : socketPath_(sockPath), pool_(workers) {}
Server::~Server()
{
    Stop();
}

void Server::Start()
{
    LOG_INFO << "Server starting";
    running_ = true;
    pool_.Start();
    loopThread_ = std::thread(&Server::Loop, this);
}

void Server::Stop()
{
    LOG_INFO << "Server stopping";
    running_ = false;
    if (loopThread_.joinable()) {
        loopThread_.join();
    }
    pool_.Stop();
    LOG_INFO << "Server stopped";
}

bool Server::InitListenSocket()
{
    listenFd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        LOG_ERROR << "socket() failed";
        return false;
    }
    SetNonBlock(listenFd_);

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socketPath_.c_str());
    unlink(socketPath_.c_str());

    if (bind(listenFd_, static_cast<sockaddr *>(static_cast<void *>(&addr)), sizeof(addr)) < 0 ||
              listen(listenFd_, LISTEN_BACK_LOG) < 0) {
        LOG_ERROR << "bind/listen failed" << strerror(errno);
        if (listenFd_ >= 0) {
            close(listenFd_);
            listenFd_ = -1;
        }
        return false;
    }

    LOG_INFO << "listening for connections on " << socketPath_;
    return true;
}

bool Server::InitEpoll()
{
    epollFd_ = epoll_create1(0);
    if (epollFd_ < 0) {
        LOG_ERROR << "epoll_create1() failed";
        if (listenFd_ >= 0) {
            close(listenFd_);
            listenFd_ = -1;
        }
        return false;
    }

    epoll_event ev{EPOLLIN, {.fd = listenFd_}};
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, listenFd_, &ev) < 0) {
        LOG_ERROR << "epoll_ctl ADD listenFd failed";
        if (listenFd_ >= 0) {
            close(listenFd_);
            listenFd_ = -1;
        }
        if (epollFd_ >= 0) {
            close(epollFd_);
            epollFd_ = -1;
        }
        return false;
    }
    return true;
}

void Server::AcceptClients()
{
    bool keepReading = true;
    while (keepReading) {
        int client = accept(listenFd_, nullptr, nullptr);
        if (client < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                keepReading = false;
                continue;
            }
            LOG_WARN << "accept() failed: " << strerror(errno);
            keepReading = false;
            continue;
        }

        SetNonBlock(client);
        conns_.emplace(client, client);
        epoll_event cev{};
        cev.events = EPOLLIN | EPOLLET;
        cev.data.fd = client;
        if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, client, &cev) < 0) {
            const int closeFd = client;
            LOG_WARN << "epoll_ctl ADD client failed: fd=" << closeFd;
            close(client);
            client = -1;
            conns_.erase(closeFd);
            continue;
        }
        LOG_INFO << "accepted client, fd=" << client;
    }
}

bool Server::HandleReadEvent(Connection &conn, int fd)
{
    auto nowSec =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t lastSec = lastSecond_.load();
    if (nowSec != lastSec) {
        lastSecond_ = nowSec;
        reqInCurrentSecond_ = 0;
    }
    if (++reqInCurrentSecond_ > qpsLimit_) {
        LOG_WARN << "Rate limit exceeded, drop request, fd=" << fd;
        return true;
    }

    bool keepReading = true;
    while (conn.HasRequest()) {
        if (!conn.HandleRead()) {
            return false;
        }
        if (!conn.HasRequest()) {
            keepReading = false;
            continue;
        }

        std::string req = conn.TakeRequest();
        if (!pool_.TryEnqueue([this, fd, req = std::move(req)] { this->HandleBusiness(fd, std::move(req)); })) {
            LOG_WARN << "ThreadPool full, drop request, fd=" << fd;
            return false;
        }
    }
    return true;
}

bool Server::HandleWriteEvent(Connection &conn, int fd) const
{
    while (conn.NeedWrite()) {
        if (!conn.HandleWrite()) {
            return false;
        }
    }

    if (!conn.NeedWrite()) {
        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = fd;
        if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev) < 0) {
            LOG_ERROR << "epoll_ctl MOD failed in HandleWriteEvent, fd=" << fd;
            return false;
        }
        conn.ResetAfterWrite();
    }
    return true;
}

void Server::CloseConnection(int fd)
{
    const int closeFd = fd;
    epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    fd = -1;
    conns_.erase(closeFd);
    LOG_INFO << "closed fd=" << closeFd;
}

void Server::HandleBusiness(int fd, const std::string &req)
{
    IpcRequest cr;
    VirtMsgUnPacker unpacker(req);
    if (cr.Deserialize(unpacker) != 0) {
        LOG_ERROR << "Failed to deserialize request, fd=" << fd;
        return;
    }
    LOG_DEBUG << "IpcRequest deserialized, service=" << cr.service_ << ", method=" << cr.method_
              << ", payload_size=" << cr.payload_.size();

    IpcResponse resp(static_cast<int32_t>(VirtIPCCode::OK));
    try {
        resp = dispatcher_.Dispatch(cr);
    } catch (const std::exception &e) {
        LOG_ERROR << "Dispatch request failed: " << e.what();
        resp.code_ = static_cast<int32_t>(VirtIPCCode::INTERNAL_ERROR);
    }

    VirtMsgPacker packer;
    if (resp.Serialize(packer) != 0) {
        LOG_ERROR << "Failed to serialize request, fd=" << fd;
        return;
    }

    auto it = conns_.find(fd);
    if (it == conns_.end()) {
        LOG_WARN << "Connection not found for fd=" << fd;
        return;
    }

    it->second.SetResponse(packer.String(), epollFd_);

    LOG_DEBUG << "IpcResponse serialized, fd=" << fd << ", code=" << resp.code_
              << ", payload_size=" << resp.payload_.size();
}

void Server::Loop()
{
    if (!InitListenSocket()) {
        return;
    }
    if (!InitEpoll()) {
        return;
    }

    epoll_event events[MAX_EPOLL_EVENTS];
    while (running_) {
        int n = epoll_wait(epollFd_, events, MAX_EPOLL_EVENTS, EPOLL_WAIT_TIMEOUT);
        if (n < 0) {
            if (errno != EINTR) {
                LOG_ERROR << "epoll_wait failed: " << strerror(errno);
            }
            continue;
        }
        if (n == 0) {
            continue;
        }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            uint32_t evt = events[i].events;

            if (fd == listenFd_) {
                AcceptClients();
                continue;
            }

            auto it = conns_.find(fd);
            if (it == conns_.end()) {
                continue;
            }
            auto &conn = it->second;

            if ((evt & EPOLLIN) && !HandleReadEvent(conn, fd)) {
                CloseConnection(fd);
                continue;
            }
            if ((evt & EPOLLOUT) && !HandleWriteEvent(conn, fd)) {
                CloseConnection(fd);
                continue;
            }
        }
    }

    if (listenFd_ >= 0) {
        close(listenFd_);
        listenFd_ = -1;
    }
    if (epollFd_ >= 0) {
        close(epollFd_);
        epollFd_ = -1;
    }
    LOG_INFO << "Event loop exited";
}
} // namespace virt::ovs::ipc::server