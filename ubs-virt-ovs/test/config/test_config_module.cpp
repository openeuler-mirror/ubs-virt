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

#include "test_config_module.h"
#include "config_module.cpp"

#include <fstream>

using namespace virt::ovs::config;
namespace ovs::ut {

void TestConfigModule::SetUp() {}

void TestConfigModule::TearDown() {}

TEST_F(TestConfigModule, IsValidNumber_ValidInteger)
{
    EXPECT_TRUE(IsValidNumber("123", false));
    EXPECT_TRUE(IsValidNumber("0", false));
    EXPECT_TRUE(IsValidNumber("999999", false));
}

TEST_F(TestConfigModule, IsValidNumber_InvalidInteger)
{
    EXPECT_FALSE(IsValidNumber("-1", false));
    EXPECT_FALSE(IsValidNumber("12.34", false));
    EXPECT_FALSE(IsValidNumber("abc", false));
    EXPECT_FALSE(IsValidNumber("12abc", false));
    EXPECT_FALSE(IsValidNumber("01", false));
}

TEST_F(TestConfigModule, IsValidNumber_ValidFloat)
{
    EXPECT_TRUE(IsValidNumber("3.14", true));
    EXPECT_TRUE(IsValidNumber("0.5", true));
    EXPECT_TRUE(IsValidNumber("-2.5", true));
    EXPECT_TRUE(IsValidNumber("100", true));
    EXPECT_TRUE(IsValidNumber("-0", true));
}

TEST_F(TestConfigModule, IsValidNumber_InvalidFloat)
{
    EXPECT_FALSE(IsValidNumber("abc", true));
    EXPECT_FALSE(IsValidNumber("12.34.56", true));
}

TEST_F(TestConfigModule, TrimConf_BasicTrim)
{
    auto [section, key, value] = TrimConf("  section  ", "  key  ");
    EXPECT_EQ(section, "section");
    EXPECT_EQ(key, "key");
    EXPECT_EQ(value, "");
}

TEST_F(TestConfigModule, TrimConf_WithValue)
{
    auto [section, key, value] = TrimConf("sec", "key", "  value  ");
    EXPECT_EQ(section, "sec");
    EXPECT_EQ(key, "key");
    EXPECT_EQ(value, "value");
}

TEST_F(TestConfigModule, TrimConf_NoTrimNeeded)
{
    auto [section, key, value] = TrimConf("section", "key", "value");
    EXPECT_EQ(section, "section");
    EXPECT_EQ(key, "key");
    EXPECT_EQ(value, "value");
}

TEST_F(TestConfigModule, GetConf_UnsupportedType)
{
    int val = 0;
    EXPECT_EQ(ConfigModule::GetInstance().GetConf<int>("sec", "key", val), ConfigCode::VALUE_TYPE_NOT_SUPPORTED);
}

} // namespace ovs::ut