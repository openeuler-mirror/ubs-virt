/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * ubs-virt-ovs is licensed under Mulan PSL v2.
 */
#ifndef UBSVIRTOVS_TEST_URMA_UTILITY_H
#define UBSVIRTOVS_TEST_URMA_UTILITY_H

#include <dlfcn.h>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

namespace ovs::ut
{
class TestUrmaUtility : public testing::Test
{
public:
    TestUrmaUtility() = default;
    void SetUp() override;
    void TearDown() override;
};
}

#endif
