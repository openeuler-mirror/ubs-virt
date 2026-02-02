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

#ifndef UBS_VIRT_OVS_MSG_H
#define UBS_VIRT_OVS_MSG_H

#include <string>
#include "virt_ipc_code.h"
#include "virt_msg_packer.h"

namespace virt::ovs::msg {

struct MsgBase {
    virtual void Serialize(VirtMsgPacker &packer) const = 0;
    virtual void Deserialize(VirtMsgUnPacker &unpacker) = 0;
    virtual VirtIPCCode Validate(std::string &errMsg)
    {
        return VirtIPCCode::OK;
    };
    virtual ~MsgBase() = default;
};

struct IpcRequest : MsgBase {
    std::string service_;
    std::string method_;
    std::string payload_;

    IpcRequest() = default;

    void Serialize(VirtMsgPacker &packer) const override
    {
        packer.Serialize(service_);
        packer.Serialize(method_);
        packer.Serialize(payload_);
    }

    void Deserialize(VirtMsgUnPacker &unpacker) override
    {
        unpacker.Deserialize(service_);
        unpacker.Deserialize(method_);
        unpacker.Deserialize(payload_);
    }
};

struct IpcResponse : MsgBase {
    uint32_t code_{};
    std::string payload_{};

    IpcResponse(const int32_t code, std::string payload) : code_(code), payload_(std::move(payload)) {};

    explicit IpcResponse(const int32_t code) : code_(code) {};

    void Serialize(VirtMsgPacker &packer) const override
    {
        packer.Serialize(code_);
        packer.Serialize(payload_);
    }

    void Deserialize(VirtMsgUnPacker &unpacker) override
    {
        unpacker.Deserialize(code_);
        unpacker.Deserialize(payload_);
    }
};

struct BaseResponse : MsgBase {
    VirtIPCCode ret_{};
    std::string message_;

    BaseResponse() = default;

    BaseResponse(const VirtIPCCode ret, const std::string &message) : ret_(ret), message_(message) {}

    void Serialize(VirtMsgPacker &packer) const override
    {
        packer.Serialize(ret_);
        packer.Serialize(message_);
    }

    void Deserialize(VirtMsgUnPacker &unpacker) override
    {
        unpacker.Deserialize(ret_);
        unpacker.Deserialize(message_);
    }

    static BaseResponse Success()
    {
        return BaseResponse(VirtIPCCode::OK, std::string{});
    }
};

struct UrmaBandwidthSetRequest : MsgBase {
    std::string name_;
    uint32_t minBandwidth_{};
    uint32_t maxBandwidth_{};

    UrmaBandwidthSetRequest() = default;

    UrmaBandwidthSetRequest(const std::string &name, const uint32_t minBandwidth, const uint32_t maxBandwidth)
        : name_(name),
          minBandwidth_(minBandwidth),
          maxBandwidth_(maxBandwidth)
    {
    }

    void Serialize(VirtMsgPacker &packer) const override
    {
        packer.Serialize(name_);
        packer.Serialize(minBandwidth_);
        packer.Serialize(maxBandwidth_);
    }

    void Deserialize(VirtMsgUnPacker &unpacker) override
    {
        unpacker.Deserialize(name_);
        unpacker.Deserialize(minBandwidth_);
        unpacker.Deserialize(maxBandwidth_);
    }

    VirtIPCCode Validate(std::string &errMsg) override
    {
        constexpr uint32_t nameMaxLength = 31;
        constexpr uint32_t minMaxLength = 1;
        constexpr uint32_t maxBandwidth = 50;
        if (name_.empty() || name_.size() > nameMaxLength) {
            errMsg = "Invalid bandwidth name,length must be 1~31 characters";
            return VirtIPCCode::INVALID_PARAM;
        }
        if (minBandwidth_ < minMaxLength || minBandwidth_ > maxBandwidth) {
            errMsg = "Invalid bandwidth minimum,must be in range[1,50]";
            return VirtIPCCode::INVALID_PARAM;
        }
        if (maxBandwidth_ < minMaxLength || maxBandwidth_ > maxBandwidth) {
            errMsg = "Invalid bandwidth maximum,must be in range[1,50]";
            return VirtIPCCode::INVALID_PARAM;
        }
        if (minBandwidth_ > maxBandwidth_) {
            errMsg = "minBandwidth cannot be greater than maxBandwidth";
            return VirtIPCCode::INVALID_PARAM;
        }
        return VirtIPCCode::OK;
    }
};

struct UrmaBandwidthResetRequest : MsgBase {
    std::string name_;

    UrmaBandwidthResetRequest() = default;

    UrmaBandwidthResetRequest(const std::string &name) : name_(name) {}

    void Serialize(VirtMsgPacker &packer) const override
    {
        packer.Serialize(name_);
    }

    void Deserialize(VirtMsgUnPacker &unpacker) override
    {
        unpacker.Deserialize(name_);
    }

    VirtIPCCode Validate(std::string &errMsg) override
    {
        constexpr uint32_t nameMaxLength = 31;
        if (name_.empty() || name_.size() > nameMaxLength) {
            errMsg = "Invalid bandwidth name,length must be 1~31 characters";
            return VirtIPCCode::INVALID_PARAM;
        }
        return VirtIPCCode::OK;
    }
};
} // namespace virt::ovs::msg
#endif // UBS_VIRT_OVS_MSG_H