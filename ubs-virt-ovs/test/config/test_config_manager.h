/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * ubs-virt-ovs is licensed under Mulan PSL v2.
 */
#ifndef UBSVIRTOVS_TEST_CONFIG_MANAGER_H
#define UBSVIRTOVS_TEST_CONFIG_MANAGER_H

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

namespace ovs::ut
{
class TestConfigManager : public testing::Test
{
public:
    TestConfigManager() = default;
    void SetUp() override;
    void TearDown() override;
};
}

#endif
