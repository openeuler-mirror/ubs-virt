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

#include "test_config_manager.h"
#include "config_manager.cpp"

#include <fstream>

using namespace virt::ovs::config;
namespace ovs::ut {

void TestConfigManager::SetUp() {}

void TestConfigManager::TearDown() {}

TEST_F(TestConfigManager, Trim_EmptyString)
{
    EXPECT_EQ(Trim(""), "");
}

TEST_F(TestConfigManager, Trim_WhitespaceOnly)
{
    EXPECT_EQ(Trim("   "), "");
    EXPECT_EQ(Trim("\t\n"), "");
}

TEST_F(TestConfigManager, Trim_LeadingAndTrailingSpaces)
{
    EXPECT_EQ(Trim("  hello  "), "hello");
    EXPECT_EQ(Trim("\tvalue\n"), "value");
}

TEST_F(TestConfigManager, Trim_NoTrimNeeded)
{
    EXPECT_EQ(Trim("test"), "test");
}

TEST_F(TestConfigManager, CatString_EmptyVector)
{
    std::vector<std::string> empty;
    EXPECT_EQ(CatString(empty, ","), "");
}

TEST_F(TestConfigManager, CatString_SingleElement)
{
    std::vector<std::string> vec = {"one"};
    EXPECT_EQ(CatString(vec, ","), "one");
}

TEST_F(TestConfigManager, CatString_MultipleElements)
{
    std::vector<std::string> vec = {"a", "b", "c"};
    EXPECT_EQ(CatString(vec, ","), "a,b,c");
}

TEST_F(TestConfigManager, FormatErrorMessage_BasicMessage)
{
    std::string result = FormatErrorMessage("Error", 10);
    EXPECT_TRUE(result.find("Error") != std::string::npos);
    EXPECT_TRUE(result.find("10") != std::string::npos);
}

TEST_F(TestConfigManager, FormatErrorMessage_WithSection)
{
    std::string result = FormatErrorMessage("Error", 5, "section1");
    EXPECT_TRUE(result.find("section1") != std::string::npos);
}

TEST_F(TestConfigManager, FormatErrorMessage_WithAllParams)
{
    std::string result = FormatErrorMessage("Error", 1, "sec", "key", "val");
    EXPECT_TRUE(result.find("sec") != std::string::npos);
    EXPECT_TRUE(result.find("key") != std::string::npos);
    EXPECT_TRUE(result.find("val") != std::string::npos);
}

TEST_F(TestConfigManager, CheckNoIllegalChars_ValidString)
{
    EXPECT_TRUE(CheckNoIllegalChars("abc123"));
    EXPECT_TRUE(CheckNoIllegalChars("test_value-1"));
    EXPECT_TRUE(CheckNoIllegalChars("name.test"));
}

TEST_F(TestConfigManager, CheckNoIllegalChars_EmptyString)
{
    EXPECT_TRUE(CheckNoIllegalChars(""));
}

TEST_F(TestConfigManager, CheckNoIllegalChars_InvalidChars)
{
    EXPECT_FALSE(CheckNoIllegalChars("test value"));
    EXPECT_FALSE(CheckNoIllegalChars("test@value"));
    EXPECT_FALSE(CheckNoIllegalChars("test!value"));
}

TEST_F(TestConfigManager, CheckNoIllegalChars_ConfigValueValid)
{
    EXPECT_TRUE(CheckNoIllegalChars("192.168.1.1", true));
    EXPECT_TRUE(CheckNoIllegalChars("value:123", true));
    EXPECT_TRUE(CheckNoIllegalChars("a/b/c", true));
}

TEST_F(TestConfigManager, IsConfFile_ValidConfFile)
{
    EXPECT_TRUE(IsConfFile("test.conf"));
    EXPECT_TRUE(IsConfFile("config.conf"));
}

TEST_F(TestConfigManager, IsConfFile_InvalidConfFile)
{
    EXPECT_FALSE(IsConfFile("test.txt"));
    EXPECT_FALSE(IsConfFile("test.cfg"));
    EXPECT_FALSE(IsConfFile(".conf"));
}

TEST_F(TestConfigManager, PathJoin_BaseWithSlash)
{
    EXPECT_EQ(PathJoin("/tmp/", "file.conf"), "/tmp/file.conf");
}

TEST_F(TestConfigManager, PathJoin_BaseWithoutSlash)
{
    EXPECT_EQ(PathJoin("/tmp", "file.conf"), "/tmp/file.conf");
}

TEST_F(TestConfigManager, PathJoin_EmptyBase)
{
    EXPECT_EQ(PathJoin("", "file.conf"), "file.conf");
}

TEST_F(TestConfigManager, CheckParamValidation_SectionTooLong)
{
    std::string longSection(CONFIG_SECTION_MAX_FIELD_LENGTH + 1, 'a');
    EXPECT_EQ(CheckParamValidation(longSection, "key", ""), ConfigCode::SECTION_LENGTH_INVALID);
}

TEST_F(TestConfigManager, CheckParamValidation_SectionTooShort)
{
    EXPECT_EQ(CheckParamValidation("", "key", ""), ConfigCode::SECTION_LENGTH_INVALID);
}

TEST_F(TestConfigManager, CheckParamValidation_KeyTooLong)
{
    std::string longKey(CONFIG_KEY_MAX_FIELD_LENGTH + 1, 'a');
    EXPECT_EQ(CheckParamValidation("sec", longKey, ""), ConfigCode::KEY_LENGTH_INVALID);
}

TEST_F(TestConfigManager, CheckParamValidation_KeyTooShort)
{
    EXPECT_EQ(CheckParamValidation("sec", "", ""), ConfigCode::KEY_LENGTH_INVALID);
}

TEST_F(TestConfigManager, CheckParamValidation_ValueTooLong)
{
    std::string longValue(CONFIG_VALUE_MAX_FIELD_LENGTH + 1, 'a');
    EXPECT_EQ(CheckParamValidation("sec", "key", longValue, true), ConfigCode::VALUE_LENGTH_INVALID);
}

TEST_F(TestConfigManager, CheckParamValidation_ValidParams)
{
    EXPECT_EQ(CheckParamValidation("section", "key", "value"), ConfigCode::OK);
}

TEST_F(TestConfigManager, TravelDepthLimitedFiles_DirectoryOpenError)
{
    std::vector<std::string> filePaths;
    MOCKER(opendir).stubs().will(returnValue((DIR *)nullptr));
    EXPECT_EQ(TravelDepthLimitedFiles(filePaths, "/nonexistent", 0), ConfigCode::CONFIG_FOLDER_OPEN_ERROR);
    GlobalMockObject::verify();
    MOCKER(opendir).reset();
}

TEST_F(TestConfigManager, TravelDepthLimitedFiles_MaxDepthExceeded)
{
    std::vector<std::string> filePaths;
    EXPECT_EQ(TravelDepthLimitedFiles(filePaths, "/tmp", CONFIG_DIR_MAX_DEPTH + 1), ConfigCode::CONFIG_FOLDER_MAX_DEPTH);
}

TEST_F(TestConfigManager, Format_IsSectionStart)
{
    Format fmt;
    EXPECT_TRUE(fmt.IsSectionStart("["));
    EXPECT_FALSE(fmt.IsSectionStart("]"));
}

TEST_F(TestConfigManager, Format_IsSectionEnd)
{
    Format fmt;
    EXPECT_TRUE(fmt.IsSectionEnd("]"));
    EXPECT_FALSE(fmt.IsSectionEnd("["));
}

TEST_F(TestConfigManager, Format_IsAssign)
{
    Format fmt;
    EXPECT_TRUE(fmt.IsAssign("="));
    EXPECT_FALSE(fmt.IsAssign(":"));
}

TEST_F(TestConfigManager, Format_IsComment)
{
    Format fmt;
    EXPECT_TRUE(fmt.IsComment(";"));
    EXPECT_TRUE(fmt.IsComment("#"));
    EXPECT_FALSE(fmt.IsComment("/"));
}

} // namespace ovs::ut