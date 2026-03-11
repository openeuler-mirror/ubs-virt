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


#include "config_module.h"
#include "config_manager.h"


using namespace virt::ovs::config;

namespace ovs::ut {

void TestConfigModule::SetUp()
{}
void TestConfigModule::TearDown()
{
    GlobalMockObject::verify();
    GlobalMockObject::reset();
}

TEST_F(TestConfigModule, IsValidNumber)
{
    EXPECT_TRUE(IsValidNumber("123", false));
    EXPECT_FALSE(IsValidNumber("-123", false));
    EXPECT_TRUE(IsValidNumber("-123.45", true));
    EXPECT_FALSE(IsValidNumber("abc", false));
}

TEST_F(TestConfigModule, TrimConf)
{
    auto [sec, key, val] = TrimConf(" sec ", " key ", " val ");
    EXPECT_EQ(sec, "sec");
    EXPECT_EQ(key, "key");
    EXPECT_EQ(val, "val");
}

TEST_F(TestConfigModule, GetConf_Types)
{
    auto& mgr = ConfigManager::GetInstance();
    mgr.configMap["type_sec"]["uint_key"] = "42";
    mgr.configMap["type_sec"]["ulong_key"] = "10000000000";
    mgr.configMap["type_sec"]["float_key"] = "3.14";
    mgr.configMap["type_sec"]["str_key"] = "hello";
    mgr.configMap["type_sec"]["bool_key"] = "true";

    ConfigModule mod;
    uint32_t uval;
    EXPECT_EQ(mod.GetConf("type_sec", "uint_key", uval), ConfigCode::OK);
    EXPECT_EQ(uval, 42u);

    uint64_t ulval;
    EXPECT_EQ(mod.GetConf("type_sec", "ulong_key", ulval), ConfigCode::OK);
    EXPECT_EQ(ulval, 10000000000ULL);

    float fval;
    EXPECT_EQ(mod.GetConf("type_sec", "float_key", fval), ConfigCode::OK);
    EXPECT_FLOAT_EQ(fval, 3.14f);

    std::string sval;
    EXPECT_EQ(mod.GetConf("type_sec", "str_key", sval), ConfigCode::OK);
    EXPECT_EQ(sval, "hello");

    bool bval;
    EXPECT_EQ(mod.GetConf("type_sec", "bool_key", bval), ConfigCode::OK);
    EXPECT_TRUE(bval);
}

TEST_F(TestConfigModule, GetConf_InvalidType)
{
    ConfigModule mod;
    int unsupported;
    EXPECT_EQ(mod.GetConf("type_sec", "any", unsupported), ConfigCode::VALUE_TYPE_NOT_SUPPORTED);
}
}
