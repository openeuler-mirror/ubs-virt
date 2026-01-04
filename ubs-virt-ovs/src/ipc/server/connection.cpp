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
#include "connection.h"
#include "logger.h"

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace virt::ovs::ipc::server {
Connection::Connection(int fd) : fd_(fd) {}

bool Connection::HandleRead()
{
    if (state_ == State::READ_LEN) {
        uint32_t lenBE;
        ssize_t n = read(fd_, &lenBE, sizeof(lenBE));
        if (n <= 0) {
            return false;
        }

        expectLen_ = ntohl(lenBE);
        readBuf_.clear();
        state_ = State::READ_BODY;
    }

    if (state_ == State::READ_BODY) {
        char buf[4096];
        ssize_t n = read(fd_, buf, sizeof(buf));
        if (n <= 0) {
            return false;
        }

        readBuf_.append(buf, n);
        if (readBuf_.size() >= expectLen_) {
            state_ = State::READY;
        }
    }

    return true;
}

bool Connection::HandleWrite()
{
    if (writeBuf_.empty()) {
        return true;
    }
    ssize_t n = write(fd_, writeBuf_.data(), writeBuf_.size());
    if (n <= 0) {
        return false;
    }

    if (writeBuf_.empty()) {
        state_ = State::READ_LEN;
    }
    return true;
}

bool Connection::NeedWrite() const
{
    return !writeBuf_.empty();
}

bool Connection::HasRequest() const
{
    return state_ == State::READY;
}

std::string Connection::TakeRequest()
{
    state_ = State::PROCESSING;
    return std::move(readBuf_);
}

void Connection::SetResponse(std::string resp, int epollFd)
{
    uint32_t lenBE = htonl(static_cast<uint32_t>(resp.size()));

    writeBuf_.resize(sizeof(lenBE));
    std::memcpy(writeBuf_.data(), &lenBE, sizeof(lenBE));
    writeBuf_.append(resp);
    state_ = State::WRITE_RESP;

    epoll_event ev{};
    ev.events = EPOLLOUT | EPOLLET;
    ev.data.fd = fd_;
    if (epoll_ctl(epollFd, EPOLL_CTL_MOD, fd_, &ev) < 0) {
        LOG_ERROR << "epoll_ctl MOD failed in SetReaponse, fd=" << fd_ << ", errno=" << errno
                  << ", errmsg=" << strerror(errno);
    }
}

void Connection::ResetAfterWrite()
{
    state_ = State::READ_LEN;
}
} // namespace virt::ovs::ipc::server