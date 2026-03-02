/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * ubs-virt-ovs is licensed under Mulan PSL v2.
 */
#include "test_config_manager.h"

#define private public
#include "config_manager.h"
#undef private

#include <fstream>
#include <filesystem>

using namespace virt::ovs::config;

namespace ovs::ut
{

void TestConfigManager::SetUp()
{}
void TestConfigManager::TearDown()
{
    GlobalMockObject::verify();
    GlobalMockObject::reset();
}

TEST_F(TestConfigManager, Trim_Basic)
{
    EXPECT_EQ(Trim("  hello  "), "hello");
    EXPECT_EQ(Trim(""), "");
    EXPECT_EQ(Trim(" \t\n "), "");
}

TEST_F(TestConfigManager, CatString_Basic)
{
    std::vector<std::string> vec = {"a", "b", "c"};
    EXPECT_EQ(CatString(vec, ","), "a,b,c");
    EXPECT_EQ(CatString({}, ","), "");
}

TEST_F(TestConfigManager, CheckNoIllegalChars_Basic)
{
    EXPECT_TRUE(CheckNoIllegalChars("valid_key-1"));
    EXPECT_FALSE(CheckNoIllegalChars("invalid key"));
    EXPECT_TRUE(CheckNoIllegalChars("192.168.1.1", true));
}

TEST_F(TestConfigManager, PathJoin_Basic)
{
    EXPECT_EQ(PathJoin("/tmp", "file.conf"), "/tmp/file.conf");
    EXPECT_EQ(PathJoin("/tmp/", "file.conf"), "/tmp/file.conf");
    EXPECT_EQ(PathJoin("", "file.conf"), "file.conf");
}

TEST_F(TestConfigManager, IsConfFile_Basic)
{
    EXPECT_TRUE(IsConfFile("test.conf"));
    EXPECT_FALSE(IsConfFile("test.txt"));
}

TEST_F(TestConfigManager, Format_Checks)
{
    Format fmt;
    EXPECT_TRUE(fmt.IsSectionStart("["));
    EXPECT_TRUE(fmt.IsSectionEnd("]"));
    EXPECT_TRUE(fmt.IsAssign("="));
    EXPECT_TRUE(fmt.IsComment(";"));
    EXPECT_TRUE(fmt.IsComment("#"));
}

TEST_F(TestConfigManager, ParseConf_Valid)
{
    ConfigManager mgr;
    std::string tempSection = "section1";
    mgr.ParseConf("file.conf", "key1=value1", 1, tempSection);
    
    std::string val;
    EXPECT_EQ(mgr.GetConf("section1", "key1", val), ConfigCode::OK);
    EXPECT_EQ(val, "value1");
}

TEST_F(TestConfigManager, ParseSection_Valid)
{
    ConfigManager mgr;
    std::string tempSection = "";
    mgr.ParseSection("file.conf", "[section2]", 1, tempSection);
    EXPECT_EQ(tempSection, "section2");
    EXPECT_TRUE(mgr.configMap.find("section2") != mgr.configMap.end());
}

TEST_F(TestConfigManager, GetConf_SectionNotExist)
{
    ConfigManager mgr;
    std::string val;
    EXPECT_EQ(mgr.GetConf("not_exist", "key", val), ConfigCode::SECTION_NOT_EXIST);
}

TEST_F(TestConfigManager, GetConf_KeyNotExist)
{
    ConfigManager mgr;
    mgr.configMap["sec1"] = {};
    std::string val;
    EXPECT_EQ(mgr.GetConf("sec1", "not_exist", val), ConfigCode::CONFIG_KEY_NOT_EXIST);
}

TEST_F(TestConfigManager, ReadConfFile_Success)
{
    std::string testFile = "test_read.conf";
    std::ofstream out(testFile);
    out << "[sec1]\nkey1=val1\n# comment\nkey2=val2\n";
    out.close();

    ConfigManager mgr;
    EXPECT_EQ(mgr.ReadConfFile(testFile), ConfigCode::OK);
    
    std::string val;
    EXPECT_EQ(mgr.GetConf("sec1", "key1", val), ConfigCode::OK);
    EXPECT_EQ(val, "val1");
    EXPECT_EQ(mgr.GetConf("sec1", "key2", val), ConfigCode::OK);
    EXPECT_EQ(val, "val2");

    std::filesystem::remove(testFile);
}

TEST_F(TestConfigManager, TravelDepthLimitedFiles_Success)
{
    std::filesystem::create_directory("test_conf_dir");
    std::ofstream out("test_conf_dir/test.conf");
    out << "[sec1]\nkey=val\n";
    out.close();

    std::vector<std::string> paths;
    EXPECT_EQ(TravelDepthLimitedFiles(paths, "test_conf_dir", 0), ConfigCode::OK);
    EXPECT_EQ(paths.size(), 1u);

    std::filesystem::remove_all("test_conf_dir");
}
}
