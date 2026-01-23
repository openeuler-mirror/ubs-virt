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

#ifndef UBS_VIRT_OVS_URMA_SERVICE_H
#define UBS_VIRT_OVS_URMA_SERVICE_H
#include "service_base.h"
#include "urma_utility.h"
namespace virt::ovs::service::urma {

using namespace virt::ovs::msg;
using namespace virt::ovs::ubse::urma;

class UrmaService : public Service {
public:
    std::string Name() const override;
    UrmaService();

private:
    IpcResponse HandleSetBandwidth(const std::string &payload);
    IpcResponse HandleResetBandwidth(const std::string &payload);
    IpcResponse HandleUpdateBandwidth(const std::string &payload);

private:
    UrmaUtility& urmaUtil_;
};

} // namespace virt::ovs::service::urma
#endif // UBS_VIRT_OVS_URMA_SERVICE_H
