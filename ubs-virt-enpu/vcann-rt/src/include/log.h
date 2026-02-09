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
#ifndef __LOG_H__
#define __LOG_H__

#include <libgen.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define ZIP_EXT ".tar.gz"
#define TAR_CMD_PREFIX "tar -czpf "
#define RM_CMD_PREFIX "rm -f "
#define MKDIR_CMD_PREFIX "mkdir -p "
#define LOG_FILE_SUFFIX ".log"
#define MODULE_NAME "eNPU"
#define SUB_MODULE_NAME "vCANN_RT"
#define MAX_CMD_LEN 1024
#define FILE_PATH_LEN 256
#define SET_UMASK_FOR_666 0000
#define SET_UMASK_FOR_440 0026
#define LOG_FILE_RIGHT 0640

extern FILE *fp1;

typedef enum {
    ENPU_LOG_FATAL = 0,
    ENPU_LOG_ERROR,
    ENPU_LOG_WARN,
    ENPU_LOG_INFO,
    ENPU_LOG_DEBUG,
} EnpuLogLevel;

typedef struct {
    char log_dir[FILE_PATH_LEN];        // 日志文件夹路径
    char log_path[FILE_PATH_LEN];       // 日志文件路径
    size_t max_file_size;       // 单个日志文件最大大小
    int max_backup_count;       // 最大未归档日志文件数
    EnpuLogLevel min_log_level;     // 最小日志打印级别
    pthread_mutex_t print_mutex;      // 日志打印锁
    pthread_mutex_t compress_mutex;     // 日志压缩锁
} LogConfig;

extern void log_print(EnpuLogLevel level, const char* filename, int line, const char* format, ...);
extern int log_init(void);
extern LogConfig g_log_config;

#define LOG_DEBUG(msg, ...) do { \
    if (g_log_config.min_log_level >= 4) { \
        log_print(ENPU_LOG_DEBUG, __FILE__, __LINE__, msg, ##__VA_ARGS__); \
        fprintf(stderr, msg"\n", ##__VA_ARGS__); \
    } \
} while (false)
#define LOG_INFO(msg, ...) do { \
    if (g_log_config.min_log_level >= 3) { \
        log_print(ENPU_LOG_INFO, __FILE__, __LINE__, msg, ##__VA_ARGS__); \
        fprintf(stderr, msg"\n", ##__VA_ARGS__); \
    } \
} while (false)
#define LOG_WARN(msg, ...) do { \
    if (g_log_config.min_log_level >= 2) { \
        log_print(ENPU_LOG_WARN, __FILE__, __LINE__, msg, ##__VA_ARGS__); \
        fprintf(stderr, msg"\n", ##__VA_ARGS__); \
    } \
} while (false)
#define LOG_ERROR(msg, ...) do { \
    if (g_log_config.min_log_level >= 1) { \
        log_print(ENPU_LOG_ERROR, __FILE__, __LINE__, msg, ##__VA_ARGS__); \
        fprintf(stderr, msg"\n", ##__VA_ARGS__); \
    } \
} while (false)
#define LOG_FATAL(msg, ...) do { \
    log_print(ENPU_LOG_FATAL, __FILE__, __LINE__, msg, ##__VA_ARGS__); \
    fprintf(stderr, msg"\n", ##__VA_ARGS__); \
} while (false)

#if defined(__cplusplus)
}
#endif

#endif