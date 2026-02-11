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
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/file.h>

#include "securec.h"
#include "common.h"
#include "log.h"

LogConfig g_log_config = {
    .log_dir = "/var/log/enpu/vcann-rt/",
    .log_path = "vCann.log",
    .max_file_size = 10 * 1024 * 1024,  // 10MB
    .max_backup_count = 10,
    .min_log_level = ENPU_LOG_INFO,
};
 
static const char* log_level_str[] = {
    "FATAL", "ERROR", "WARN", "INFO", "DEBUG"
};

// bytes
long get_file_size(const char* filepath)
{
    struct stat st;
    if (stat(filepath, &st) < 0) {
        return 0;
    }
    return st.st_size;
}

int is_current_process(const char* filename)
{
    char* token;
    char* temp = strdup(filename);
    const int tokens_num = 5;
    char* tokens[tokens_num];
    int token_count = 0;

    const char* delimiter = "_";
    token = strtok(temp, delimiter);
    while ((token != NULL) && (token_count < tokens_num)) {
        tokens[token_count++] = token;
        token = strtok(NULL, delimiter);
    }
    free(temp);

    if (token_count < tokens_num) {
        return ENPU_FAIL;
    }

    char* endptr;
    const int pid_token_index = 3;
    const int pid_base = 10;
    long pid_from_filename = strtol(tokens[pid_token_index], &endptr, pid_base);
    const char flag = '\0';
    if (*endptr != flag) {
        fprintf(stderr, "Failed to get pid from log filename. now log filename is %s.\n", g_log_config.log_path);
        return ENPU_FAIL;
    }

    pid_t current_pid = getpid();
    if (pid_from_filename == (long)current_pid) {
        return ENPU_SUCCESS;
    } else {
        return ENPU_FAIL;
    }
}

int is_log_file(const char* filename)
{
    size_t len = strlen(filename);
    size_t suffix_len = strlen(LOG_FILE_SUFFIX);
    if ((len < suffix_len) || (strcmp(filename + len - suffix_len, LOG_FILE_SUFFIX) != 0)) {
        return ENPU_FAIL;
    }

    return is_current_process(filename);
}

int count_log_files()
{
    DIR* dir = opendir(g_log_config.log_dir);
    if (dir == NULL) {
        printf("Count log files error, failed to open directory %s\n", g_log_config.log_dir);
        return -1;
    }
    int file_count = 0;
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
                return -1;
            }
            return -1;
        }
        if (stat(full_path, &statbuf) != 0) {
            continue;
        }
        if (S_ISREG(statbuf.st_mode) && (is_log_file(full_path) == ENPU_SUCCESS)) {
            file_count++;
        }
    }

    if (closedir(dir) == -1) {
        return -1;
    }
    return file_count;
}

int compress_file()
{
    char zip_file[FILE_PATH_LEN];
    char tar_cmd[MAX_CMD_LEN];
    char rm_cmd[MAX_CMD_LEN];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    if (tm_info == NULL) {
        perror("Compress files error, failed to get current time");
        return ENPU_FAIL;
    }
    char timestamp[20];
    if (strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", tm_info) == 0) {
        perror("Compress files error, failed to format timestamp");
        return ENPU_FAIL;
    }
    int ret = snprintf_s(zip_file, sizeof(zip_file), sizeof(zip_file), "%s%s_%s",
        g_log_config.log_dir, SUB_MODULE_NAME, timestamp);
    CHECK_COND_RETURN_ERROR_CODE_LOG(ret < 0, "Failed to get zip file name with timestamp.");
    ret = snprintf_s(zip_file + strlen(zip_file), sizeof(zip_file), sizeof(zip_file) - strlen(zip_file), "%s", ZIP_EXT);
    CHECK_COND_RETURN_ERROR_CODE_LOG(ret < 0, "Failed to get full zip file name.");
    umask(SET_UMASK_FOR_440); // 默认权限440
    ret = snprintf_s(tar_cmd, sizeof(tar_cmd), sizeof(tar_cmd), "%s%s %s%s %s%s%d%s", TAR_CMD_PREFIX, zip_file,
        "--exclude=", g_log_config.log_path, g_log_config.log_dir, "*_", getpid(), "_*.log");
    CHECK_COND_RETURN_ERROR_CODE_LOG(ret < 0, "Failed to get tar cmd.");
    ret = snprintf_s(rm_cmd, sizeof(rm_cmd), sizeof(rm_cmd), "%s%s %s%d%s%s%s", "find ", g_log_config.log_dir,
        "-type f -iname \"*_", getpid(), "_*.log\" ! -iwholename \"", g_log_config.log_path, "\" -exec rm -f {} \\;");
    CHECK_COND_RETURN_ERROR_CODE_LOG(ret < 0, "Failed to get rm cmd.");
    int tar_exe_result = system(tar_cmd);
    CHECK_COND_RETURN_ERROR_CODE_LOG(tar_exe_result != 0, "Compress files error, failed to execute tar command");
    int rm_exe_result = system(rm_cmd);
    CHECK_COND_RETURN_ERROR_CODE_LOG(rm_exe_result != 0, "Compress files error, failed to execute rm command");
    printf("Compressed .log files into %s and deleted the original files.\n", g_log_config.log_dir);
    return ENPU_SUCCESS;
}

int update_log_file()
{
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    char time_str[64];
    int ret = strftime(time_str, sizeof(time_str), "%Y%m%d%H%M%S", &tm_now);
    CHECK_COND_RETURN_ERROR_CODE_LOG(ret == 0, "Update log file error: failed to get timestamp.");

    char log_path[FILE_PATH_LEN];
    ret = snprintf_s(log_path, sizeof(log_path), sizeof(log_path), "%s%s_%s_%d_%s%s", g_log_config.log_dir,
        MODULE_NAME, SUB_MODULE_NAME, getpid(), time_str, LOG_FILE_SUFFIX);
    CHECK_COND_RETURN_ERROR_CODE_LOG(ret < 0, "Failed to get log file name.");
    ret = strncpy_s(g_log_config.log_path, sizeof(g_log_config.log_path), log_path, sizeof(g_log_config.log_path) - 1);
    CHECK_COND_RETURN_ERROR_CODE_LOG(ret != 0, "Failed to set g_log_config.log_path %s.", log_path);

    umask(SET_UMASK_FOR_666); // 规避系统默认文件权限不一致问题
    if (creat(g_log_config.log_path, LOG_FILE_RIGHT) < 0) {
        perror("update log file error: Create new log file");
        return ENPU_FAIL;
    }
    return ENPU_SUCCESS;
}

int rotate_log_by_size()
{
    long file_size = get_file_size(g_log_config.log_path);
    if (file_size < g_log_config.max_file_size) {
        return ENPU_SUCCESS;
    }

    int ret = update_log_file();
    CHECK_RETURN_ERROR_CODE_LOG(ret, "Failed to update log file, now log file is %s.\n", g_log_config.log_path);
    return ENPU_SUCCESS;
}

int log_init()
{
    pthread_mutex_init(&g_log_config.print_mutex, NULL);
    pthread_mutex_init(&g_log_config.compress_mutex, NULL);

    printf("dir_path: %s\n", g_log_config.log_dir);
    char mkdir_cmd[MAX_CMD_LEN];
    int ret = snprintf_s(mkdir_cmd, sizeof(mkdir_cmd), sizeof(mkdir_cmd), "%s%s",
        MKDIR_CMD_PREFIX, g_log_config.log_dir);
    CHECK_COND_RETURN_ERROR_CODE_LOG(ret < 0, "Failed to get log dir path.");
    system(mkdir_cmd);

    ret = is_current_process(g_log_config.log_path);
    if (ret != ENPU_SUCCESS) {
        ret = update_log_file();
        CHECK_RETURN_ERROR_CODE_LOG(ret, "Failed to update log file, now log file is %s.\n", g_log_config.log_path);
    }

    char* enpu_log_level = getenv("ENPU_LOG_LEVEL");
    g_log_config.min_log_level = (enpu_log_level == NULL) ? ENPU_LOG_INFO : (EnpuLogLevel)atoi(enpu_log_level);
    return ENPU_SUCCESS;
}

void log_print(EnpuLogLevel level, const char* filename, int line, const char* format, ...)
{
    if (level > g_log_config.min_log_level) {
        return;
    }
    pthread_mutex_lock(&g_log_config.print_mutex);
    int ret = rotate_log_by_size();
    if (ret != ENPU_SUCCESS) {
        pthread_mutex_unlock(&g_log_config.print_mutex);
        return;
    }
    FILE* fp = fopen(g_log_config.log_path, "a");
    if (!fp) {
        pthread_mutex_unlock(&g_log_config.print_mutex);
        return;
    }
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    char time_str[64];
    ret = strftime(time_str, sizeof(time_str), "%Y%m%d%H%M%S", &tm_now);
    CHECK_COND_RETURN_LOG(ret == 0, "Failed to get timestamp");
    ret = fprintf(fp, "[%s] [%s] [%s] [%s] [%d:%ld:%s:%d] ", time_str, log_level_str[level],
        MODULE_NAME, SUB_MODULE_NAME, getpid(), pthread_self(), basename(filename), line);
    CHECK_COND_RETURN_LOG(ret < 0, "Failed to fprint log content to file");
    ret = fprintf(stderr, "[%s] [%s] [%s] [%s] [%d:%ld:%s:%d] ", time_str, log_level_str[level],
        MODULE_NAME, SUB_MODULE_NAME, getpid(), pthread_self(), basename(filename), line);
    CHECK_COND_RETURN_LOG(ret < 0, "Failed to fprint log content to stderr");
    va_list args;
    va_start(args, format);
    ret = vfprintf(fp, format, args);
    CHECK_COND_RETURN_LOG(ret < 0, "Failed to vfprint log content to file");
    va_end(args);
    ret = fprintf(fp, "\n");
    CHECK_COND_RETURN_LOG(ret < 0, "Failed to fprint log content end to file");
    ret = fflush(fp);
    CHECK_COND_RETURN_LOG(ret < 0, "Failed to fflush log content to file");
    ret = fclose(fp);
    CHECK_COND_RETURN_LOG(ret < 0, "Failed to close log file");
    pthread_mutex_unlock(&g_log_config.print_mutex);
    pthread_mutex_lock(&g_log_config.compress_mutex);
    int log_file_count = count_log_files();
    if (log_file_count > g_log_config.max_backup_count) {
        ret = compress_file();
        CHECK_ERROR_CODE_LOG(ret, "Failed to compress log files, log_dir is %s\n", g_log_config.log_dir);
    }
    pthread_mutex_unlock(&g_log_config.compress_mutex);
}