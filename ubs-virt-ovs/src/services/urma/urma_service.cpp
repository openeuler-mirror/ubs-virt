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

#include "urma_service.h"
#include "logger.h"
#include "macros.h"
#include "protocol.h"

namespace virt::ovs::service::urma {

using namespace virt::ovs;

std::string UrmaService::Name() const
{
    return "ubs.urma";
}

UrmaService::UrmaService() : urmaUtil_(UrmaUtility::Instance())
{
    Register("SetBandwidth", &UrmaService::HandleSetBandwidth);
    Register("UpdateBandwidth", &UrmaService::HandleUpdateBandwidth);
    Register("ResetBandwidth", &UrmaService::HandleResetBandwidth);
}

IpcResponse UrmaService::HandleSetBandwidth(const std::string &payload)
{
    UrmaBandwidthSetRequest request;
    BaseResponse resp;
    std::string errMsg;
    VirtIPCCode ret = DeserializeAndValidate(request, payload, errMsg);
    if (ret != VirtIPCCode::OK) {
        LOG_ERROR << "HandleSetBandwidth failed: " << errMsg;
        return BizError<BaseResponse>(ret, errMsg);
    }
    uint32_t res = urmaUtil_.SetBandWidth(request.name_, request.minBandwidth_, request.maxBandwidth_);
    if (res != 0) {
        LOG_ERROR << "set bandwidth failed,name is " << request.name_ << ",res=" << res;
        return BizError<BaseResponse>(VirtIPCCode::UBSE_ERROR, "call ubse failed");
    }
    return Ok(BaseResponse::Success());
}

IpcResponse UrmaService::HandleUpdateBandwidth(const std::string &payload)
{
    UrmaBandwidthSetRequest request;
    BaseResponse resp;
    std::string errMsg;
    VirtIPCCode ret = DeserializeAndValidate(request, payload, errMsg);
    if (ret != VirtIPCCode::OK) {
        LOG_ERROR << "HandleUpdateBandwidth failed: " << errMsg;
        return BizError<BaseResponse>(ret, errMsg);
    }
    uint32_t minBandwidth = 0;
    uint32_t maxBandwidth = 0;
    uint32_t res = urmaUtil_.GetBandWidth(request.name_, minBandwidth, maxBandwidth);
    if (res == static_cast<uint32_t>(VirtIPCCode::NOT_EXIST)) {
        LOG_ERROR << "urma bandwidth config is not exist,name is" << request.name_;
        return BizError<BaseResponse>(VirtIPCCode::NOT_EXIST, "not exist");
    }
    if (res != 0) {
        LOG_ERROR << "get bandwidth from ubse failed,name is " << request.name_ << ",res=" << res;
        return BizError<BaseResponse>(VirtIPCCode::UBSE_ERROR, "call ubse failed");
    }
    res = urmaUtil_.DisableBandWidth(request.name_);
    if (res != 0) {
        LOG_ERROR << "disable bandwidth failed,name is " << request.name_ << ",res=" << res;
        return BizError<BaseResponse>(VirtIPCCode::UBSE_ERROR, "call ubse failed");
    }
    res = urmaUtil_.SetBandWidth(request.name_, request.minBandwidth_, request.maxBandwidth_);
    if (res != 0) {
        LOG_ERROR << "set bandwidth failed,name is " << request.name_ << ",res=" << res;
        res = urmaUtil_.SetBandWidth(request.name_, minBandwidth, maxBandwidth);
        if (res != 0) {
            LOG_ERROR << "try rollback bandwidth failed,name is " << request.name_ << ",res=" << res;
        }
        return BizError<BaseResponse>(VirtIPCCode::UBSE_ERROR, "call ubse failed");
    }
    return Ok(BaseResponse::Success());
}

IpcResponse UrmaService::HandleResetBandwidth(const std::string &payload)
{
    UrmaBandwidthResetRequest request;
    BaseResponse resp;
    std::string errMsg;
    VirtIPCCode ret = DeserializeAndValidate(request, payload, errMsg);
    if (ret != VirtIPCCode::OK) {
        LOG_ERROR << "HandleResetBandwidth failed: " << errMsg;
        return BizError<BaseResponse>(ret, errMsg);
    }
    uint32_t minBandwidth = 0;
    uint32_t maxBandwidth = 0;
    uint32_t res = urmaUtil_.GetBandWidth(request.name_, minBandwidth, maxBandwidth);
    if (res == static_cast<uint32_t>(VirtIPCCode::NOT_EXIST)) {
        LOG_WARN << "urma bandwidth config is already reset,name is" << request.name_;
        return Ok(BaseResponse::Success());
    }
    if (res != 0) {
        LOG_ERROR << "get bandwidth from ubse failed,name is " << request.name_ << ",res=" << res;
        return BizError<BaseResponse>(VirtIPCCode::UBSE_ERROR, "call ubse failed");
    }
    res = urmaUtil_.DisableBandWidth(request.name_);
    if (res != 0) {
        LOG_ERROR << "disable bandwidth failed,name is " << request.name_ << ",res=" << res;
        return BizError<BaseResponse>(VirtIPCCode::UBSE_ERROR, "call ubse failed");
    }
    return Ok(BaseResponse::Success());
}

REGISTER_SERVICE(UrmaService);

} // namespace virt::ovs::service::urma