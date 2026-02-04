/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-ovs is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/file.h>

#include "securec.h"
#include "common.h"
#include "log.h"

void remove_mock_files() {
    DIR* dir = opendir(g_log_config.log_dir);
    if (dir == NULL) {
        return;
    }
    struct dirent* entry;
    struct stat statbuf;
    while ((entry = readdir(dir)) != NULL) {
        if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)) {
            continue;
        }
        char full_path[FILE_PATH_LEN];
        int ret = snprintf_s(full_path, sizeof(full_path), sizeof(full_path), "%s%s",
            g_log_config.log_dir, entry->d_name);
        if (ret < 0) {
            if (closedir(dir) == -1) {
                return;
            }
            return;
        }
        if (stat(full_path, &statbuf) != 0) {
            continue;
        }
        if (S_ISREG(statbuf.st_mode)) {
            remove(full_path);
        }
    }

    if (closedir(dir) == -1) {
        return;
    }
}

void set_log_config(LogConfig log_config) {
    g_log_config = log_config;
}

class LogTest : public testing::Test {
protected:
    void SetUp() {
        LogConfig mock_log_config;
        strcpy_s(mock_log_config.log_dir, sizeof(mock_log_config.log_dir), "/tmp/log/enpu/vcann-rt/mock/");
        strcpy_s(mock_log_config.log_path, sizeof(mock_log_config.log_path), "vCann.log");
        mock_log_config.max_file_size = 1024 * 1024; // 1MB
        mock_log_config.max_backup_count = 3;
        mock_log_config.min_log_level = ENPU_LOG_INFO;
        set_log_config(mock_log_config);
        log_init();
    }

    void TearDown() {
        remove_mock_files();
        rmdir(g_log_config.log_dir);

        LogConfig origin_log_config;
        strcpy_s(origin_log_config.log_dir, sizeof(origin_log_config.log_dir), "/var/log/enpu/vcann-rt/");
        strcpy_s(origin_log_config.log_path, sizeof(origin_log_config.log_path), "vCann.log");
        origin_log_config.max_file_size = 10 * 1024 * 1024; // 10MB
        origin_log_config.max_backup_count = 10;
        origin_log_config.min_log_level = ENPU_LOG_INFO;
        set_log_config(origin_log_config);
    }
};

TEST_F(LogTest, LogTest_get_file_size)
{
    const char *filename = "/tmp/log/enpu/vcann-rt/mock/eNPU_vCANN_RT.log";
    const char *message = "This is a test log message from C program.\n";

    int ret = get_file_size(filename);
    EXPECT_EQ(ret, 0);

    FILE *file = fopen(filename, "w");
    ret = fprintf(file, "%s", message);
    EXPECT_EQ(ret, strlen(message));

    fflush(file);
    fclose(file);

    ret = get_file_size(filename);
    EXPECT_EQ(ret, strlen(message));

    remove(filename);
}

TEST_F(LogTest, LogTest_is_log_file)
{
    const char *filename = "/tmp/log/enpu/vcann-rt/mock/eNPU_vCANN_RT.log";
    const char *filename_error = "/tmp/log/enpu/vcann-rt/mock/eNPU_vCANN_RT.txt";

    int ret = is_log_file(filename);
    EXPECT_EQ(ret, ENPU_SUCCESS);

    ret = is_log_file(filename_error);
    EXPECT_EQ(ret, ENPU_FAIL);
}

TEST_F(LogTest, LogTest_count_log_files)
{
    remove_mock_files();

    const char *filename1 = "/tmp/log/enpu/vcann-rt/mock/eNPU_vCANN_RT_1.log";
    const char *filename2 = "/tmp/log/enpu/vcann-rt/mock/eNPU_vCANN_RT_2.log";
    const char *filename3 = "/tmp/log/enpu/vcann-rt/mock/eNPU_vCANN_RT_3.log";

    FILE *file1 = fopen(filename1, "w");
    fclose(file1);
    FILE *file2 = fopen(filename2, "w");
    fclose(file2);
    FILE *file3 = fopen(filename3, "w");
    fclose(file3);

    int ret = count_log_files();
    EXPECT_EQ(ret, 3);

    remove(filename1);
    remove(filename2);
    remove(filename3);

    strcpy_s(g_log_config.log_dir, sizeof(g_log_config.log_dir), "/tmp/log/enpu/vcann-rt/mock/test/");
    ret = count_log_files();
    EXPECT_EQ(ret, -1);
    strcpy_s(g_log_config.log_dir, sizeof(g_log_config.log_dir), "/tmp/log/enpu/vcann-rt/mock/");
}

TEST_F(LogTest, LogTest_compress_file)
{
    const char *filename1 = "/tmp/log/enpu/vcann-rt/mock/eNPU_vCANN_RT_1.log";
    const char *filename2 = "/tmp/log/enpu/vcann-rt/mock/eNPU_vCANN_RT_2.log";
    const char *filename3 = "/tmp/log/enpu/vcann-rt/mock/eNPU_vCANN_RT_3.log";

    FILE *file1 = fopen(filename1, "w");
    fclose(file1);
    FILE *file2 = fopen(filename2, "w");
    fclose(file2);
    FILE *file3 = fopen(filename3, "w");
    fclose(file3);

    int ret = compress_file();
    EXPECT_EQ(ret, ENPU_SUCCESS);

    remove_mock_files();
}

TEST_F(LogTest, LogTest_update_log_file)
{
    int ret = update_log_file();
    EXPECT_EQ(ret, ENPU_SUCCESS);

    remove_mock_files();
}

TEST_F(LogTest, LogTest_rotate_log_by_size)
{
    log_init();

    const char *message = "This is a test log message from C program.\n";
    FILE *file = fopen(g_log_config.log_path, "w");

    int ret = fprintf(file, "%s", message);
    EXPECT_EQ(ret, strlen(message));
    fflush(file);
    fclose(file);

    ret = rotate_log_by_size();
    EXPECT_EQ(ret, ENPU_SUCCESS);

    file = fopen(g_log_config.log_path, "a");
    for (int i = 0; i * strlen(message) < g_log_config.max_file_size; ++i) {
        ret = fprintf(file, "%s", message);
        EXPECT_EQ(ret, strlen(message));
    }
    fflush(file);
    fclose(file);

    ret = rotate_log_by_size();
    EXPECT_EQ(ret, ENPU_SUCCESS);

    remove_mock_files();
}