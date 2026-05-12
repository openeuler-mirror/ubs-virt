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

#include "test_logger.h"
#include "logger.cpp"

#include <securec.h>

using namespace virt::logger;
using mockcpp::atLeast;
namespace ovs::ut {

TEST_F(TestLogger, NowFilename_ReturnNonEmptyString)
{
    std::string filename = NowFilename();
    EXPECT_FALSE(filename.empty());
    EXPECT_GT(filename.size(), 0u);
    EXPECT_EQ(filename.size(), 15); // expect filename size is 15
}

TEST_F(TestLogger, LoggerEntry_SubmitDoesNotThrow)
{
    Logger entry(LoggerLevel::INFO, "logger_test.cpp", "LoggerEntry_SubmitDoesNotThrow", 123); // 123 is line num
    EXPECT_NO_THROW({
        entry << "This is a test log";
        entry.Submit();
    });
}

TEST_F(TestLogger, EnsureLogDir_MkdirFailed)
{
    MOCKER(mkdir).stubs().will(returnValue(-1));
    MOCKER(stat).stubs().will(returnValue(-1));
    EnsureLogDir();
    MOCKER(stat).reset();
    MOCKER(mkdir).reset();
}

TEST_F(TestLogger, GetFileSize_Failed)
{
    MOCKER(stat).stubs().will(returnValue(-1));
    EXPECT_EQ(GetFileSize("not_exist_file"), 0u);
    MOCKER(stat).reset();
}

TEST_F(TestLogger, NowFilename_StrftimeFailed)
{
    MOCKER(strftime).stubs().will(returnValue(size_t(0)));
    EXPECT_EQ(NowFilename(), "00000000_000000");
    MOCKER(strftime).reset();
}

static struct dirent *MakeDirent(const char *name)
{
    static struct dirent ent;
    memset_s(&ent, sizeof(ent), 0, sizeof(ent));
    strncpy_s(ent.d_name, sizeof(ent.d_name), name, sizeof(ent.d_name) - 1);
    return &ent;
}

TEST_F(TestLogger, CleanupOldRotateLogFile_NoDelete)
{
    DIR *fackDir = reinterpret_cast<DIR *>(0x1234);
    MOCKER(opendir).expects(once()).will(returnValue(fackDir));
    MOCKER(readdir)
        .expects(mockcpp::atLeast(1))
        .will(returnValue(MakeDirent("virt_ovs_20240101010101.tar.gz")))
        .then(returnValue(MakeDirent("virt_ovs_20240102010101.tar.gz")))
        .then(returnValue((dirent *)0));
    MOCKER(closedir).expects(once()).will(returnValue(1));
    MOCKER(strftime).stubs().will(returnValue(static_cast<size_t>(1)));

    MOCKER(mktime)
        .stubs()
        .will(returnValue(time_t(1)))
        .then(returnValue(time_t(2)))  // mock file stat time is 2
        .then(returnValue(time_t(3)))  // mock file stat time is 3
        .then(returnValue(time_t(4))); // mock file stat time is 4

    MOCKER(unlink).expects(never());

    CleanupOldRotateLogFile();

    GlobalMockObject::verify();
    GlobalMockObject::reset();
    MOCKER(opendir).reset();
    MOCKER(readdir).reset();
    MOCKER(closedir).reset();
    MOCKER(strftime).reset();
    MOCKER(mktime).reset();
    MOCKER(unlink).reset();
}

TEST_F(TestLogger, CleanupOldRotateLogFile_IgnoreInvalid)
{
    DIR *fackDir = reinterpret_cast<DIR *>(0x1234);
    MOCKER(opendir).expects(once()).will(returnValue(fackDir));
    MOCKER(readdir)
        .expects(mockcpp::atLeast(1))
        .will(returnValue(MakeDirent("randon.txt")))
        .then(returnValue(MakeDirent("virt_ovs_invalid.tar.gz")))
        .then(returnValue((dirent *)0));
    MOCKER(closedir).expects(once()).will(returnValue(1));
    MOCKER(strftime).stubs().will(returnValue(static_cast<size_t>(10))); // mock strftime will return 10

    MOCKER(unlink).expects(never()).will(returnValue(0));
    CleanupOldRotateLogFile();
    GlobalMockObject::verify();
    GlobalMockObject::reset();
    MOCKER(opendir).reset();
    MOCKER(readdir).reset();
    MOCKER(closedir).reset();
    MOCKER(strftime).reset();
    MOCKER(unlink).reset();
}

TEST_F(TestLogger, CompressOldLogFile_CallCleanup)
{
    MOCKER(CleanupOldRotateLogFile).stubs();
    std::string oldLogFile = "/tmp/test.log";
    std::string ts = "20260123_121314";

    EXPECT_NO_THROW({ CompressOldLogFile(oldLogFile, ts); });
    GlobalMockObject::verify();
    MOCKER(CleanupOldRotateLogFile).reset();
}

TEST_F(TestLogger, RotateLogFile_SizeNotEnough)
{
    MOCKER(GetFileSize).expects(once()).will(returnValue(MAX_LOG_SIZE - 1));
    MOCKER(CompressOldLogFile).expects(never());
    MOCKER(SetFileMode).expects(never());

    Logger::RotateLogFile();
    GlobalMockObject::verify();
    GlobalMockObject::reset();
    MOCKER(GetFileSize).reset();
    MOCKER(CompressOldLogFile).reset();
    MOCKER(SetFileMode).reset();
}

TEST_F(TestLogger, RotateLogFile_NormalRotate)
{
    MOCKER(GetFileSize).expects(once()).will(returnValue(MAX_LOG_SIZE + 1));
    MOCKER(NowFilename).expects(once()).will(returnValue(std::string("20240101_010101")));
    MOCKER(SetFileMode);
    Logger::RotateLogFile();
    GlobalMockObject::verify();
    GlobalMockObject::reset();
    MOCKER(GetFileSize).reset();
    MOCKER(NowFilename).reset();
    MOCKER(SetFileMode).reset();
}
} // namespace ovs::ut