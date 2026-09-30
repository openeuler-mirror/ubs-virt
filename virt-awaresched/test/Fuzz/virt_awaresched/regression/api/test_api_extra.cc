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

#include "../../../../vasd/api/test_api.h"

#include <map>
#include <string>

#include "mockcpp/mokc.h"

#include "api.h"
#include "cluster_sched.h"
#include "cmd.h"

using namespace vas::sched;

namespace vas::sched::ut {
TEST_F(TestApi, testReAssignSchedulerFailure)
{
    std::map<std::string, std::string> data;
    data[REASSIGN_SCOPE] = "test_scope";
    std::string resStr;
    MOCKER(&ClusterSched::ReSchedVm).stubs().will(returnValue(VAS_ERROR));
    const VasRet ret = Api::ReAssign(data, resStr);
    EXPECT_EQ(ret, VAS_ERROR);
    EXPECT_EQ(resStr, "ReAssign failed.");
    MOCKER(&ClusterSched::ReSchedVm).reset();
}
} // namespace vas::sched::ut
