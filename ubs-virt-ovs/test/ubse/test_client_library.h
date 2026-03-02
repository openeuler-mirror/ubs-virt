/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * ubs-virt-ovs is licensed under Mulan PSL v2.
 */
#ifndef UBSVIRTOVS_TEST_CLIENT_LIBRARY_H
#define UBSVIRTOVS_TEST_CLIENT_LIBRARY_H

#include <dlfcn.h>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

namespace ovs::ut {
class TestClientLibrary : public testing::Test {
public:
    TestClientLibrary() = default;
    void SetUp() override;
    void TearDown() override;
};
}

#endif
