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

#ifndef UBS_VIRT_OVS_DISPATCHER_H
#define UBS_VIRT_OVS_DISPATCHER_H

#include "service_base.h"

using namespace virt::ovs::msg;

namespace virt::ovs::ipc::server {
class Dispatcher {
public:
    IpcResponse Dispatch(const IpcRequest &request) const;
};
} // namespace virt::ovs::ipc::server
#endif // UBS_VIRT_OVS_DISPATCHER_H
