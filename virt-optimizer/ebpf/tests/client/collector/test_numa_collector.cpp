/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <mockcpp/mockcpp.hpp>
#include <mockcpp/GlobalMockObject.h>
#include "client/collector/numa_collector.h"
#include "client/collector/collector.h"
#include "common/data_struct.h"

TEST(NumaCollectorTest, GetInstanceTest)
{
    NumaCollector &obj1 = NumaCollector::getInstance();
    NumaCollector &obj2 = NumaCollector::getInstance();
    EXPECT_EQ(&obj1, &obj2);
}
