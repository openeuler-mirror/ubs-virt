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

#include "test_vas_security_manager.h"

#include <mockcpp/mockcpp.hpp>

#include "error.h"
#include "logger.h"
#include "vas_security_manager.h"

namespace vas::ut::security {
using namespace vas::common;
using namespace vas::security;
namespace {
// Mimic capget: fill capData with a known permitted set on success, so the
// permitted-set check inside ModifyEffectiveCapabilities reads deterministic
// data instead of uninitialized stack memory.
int FakeGetCapFillPermitted(__user_cap_header_struct *capHeader, __user_cap_data_struct *capData)
{
    (void)capHeader;
    if (capData == nullptr) {
        return -1;
    }
    capData[0] = {};
    capData[1] = {};
    capData[CAP_TO_INDEX(CAP_FOWNER)].permitted |= CAP_TO_MASK(CAP_FOWNER);
    capData[CAP_TO_INDEX(CAP_DAC_OVERRIDE)].permitted |= CAP_TO_MASK(CAP_DAC_OVERRIDE);
    return 0;
}
} // namespace

void TestVasSecurityManager::SetUp()
{
    Test::SetUp();
}

void TestVasSecurityManager::TearDown()
{
    GlobalMockObject::verify();
    Test::TearDown();
}

TEST_F(TestVasSecurityManager, testGetCapabilities)
{
    MOCKER(VasSecurityManager::GetCap).stubs().will(returnValue(-1)).then(returnValue(0));
    EXPECT_EQ(VasSecurityManager::GetCapabilities(), VAS_ERROR);
    EXPECT_EQ(VasSecurityManager::GetCapabilities(), VAS_OK);
    MOCKER(VasSecurityManager::GetCap).reset();
}

TEST_F(TestVasSecurityManager, testSetInitialCapabilities)
{
    MOCKER(VasSecurityManager::SetCap).stubs().will(returnValue(-1)).then(returnValue(0));
    EXPECT_EQ(VasSecurityManager::SetInitialCapabilities(), VAS_ERROR);
    EXPECT_EQ(VasSecurityManager::SetInitialCapabilities(), VAS_OK);
    MOCKER(VasSecurityManager::SetCap).reset();
}

TEST_F(TestVasSecurityManager, testModifyEffectiveCapabilities)
{
    const std::vector<__u32> caps = {
        CAP_FOWNER,
    };
    int effectiveCapabilities = 999;
    MOCKER(VasSecurityManager::GetCap).stubs().will(invoke(FakeGetCapFillPermitted));
    MOCKER(VasSecurityManager::SetCap).stubs().will(returnValue(0));
    EXPECT_EQ(VasSecurityManager::ModifyEffectiveCapabilities(caps, VasCapOperateType::CAP_ADD), VAS_OK);
    EXPECT_EQ(VasSecurityManager::ModifyEffectiveCapabilities(caps, VasCapOperateType::CAP_DELETE), VAS_OK);
    EXPECT_EQ(
        VasSecurityManager::ModifyEffectiveCapabilities(caps, static_cast<VasCapOperateType>(effectiveCapabilities)),
        VAS_ERROR_INVAL);
    MOCKER(VasSecurityManager::GetCap).reset();
    MOCKER(VasSecurityManager::SetCap).reset();
}

TEST_F(TestVasSecurityManager, testClearCapabilities)
{
    const std::vector<__u32> caps = {
        CAP_DAC_OVERRIDE,
    };
    MOCKER(VasSecurityManager::GetCap).stubs().will(invoke(FakeGetCapFillPermitted));
    MOCKER(VasSecurityManager::SetCap).stubs().will(returnValue(0));
    EXPECT_EQ(VasSecurityManager::ModifyEffectiveCapabilities(caps, VasCapOperateType::CAP_ADD), VAS_OK);
    VasSecurityManager::ClearCapabilities(caps);
    MOCKER(VasSecurityManager::GetCap).reset();
    MOCKER(VasSecurityManager::GetCap).stubs().will(returnValue(-1));
    VasSecurityManager::ClearCapabilities(caps);
    MOCKER(VasSecurityManager::GetCap).reset();
    MOCKER(VasSecurityManager::SetCap).reset();
}

} // namespace vas::ut::security