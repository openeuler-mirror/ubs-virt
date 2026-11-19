/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 */

#include "enpu_config.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "log.h"
#include "securec.h"

static void copy_config_str(char *dest, size_t dest_size, const char *src)
{
    if (src == NULL) {
        LOG_ERROR("[CONFIG] copy_config_str: src is NULL");
        return;
    }

    errno_t ret = strncpy_s(dest, dest_size, src, strnlen(src, dest_size - 1));
    if (ret != 0) {
        LOG_ERROR("[CONFIG] strncpy_s failed, ret=%d, dest_size=%zu", ret, dest_size);
    }
}

static void init_defaults(enpu_config_t *config)
{
    copy_config_str(config->socket_path, MAX_PATH_LEN, ENPU_CONFIG_DEFAULT_SOCKET_PATH);

    copy_config_str(config->log_level, MAX_NAME_LEN, "info");
    copy_config_str(config->log_dir, MAX_PATH_LEN, "/var/log/enpu-manager");
    config->log_max_size = ENPU_CONFIG_DEFAULT_LOG_MAX_SIZE;
    config->log_max_backups = ENPU_CONFIG_DEFAULT_LOG_MAX_BACKUPS;
    config->log_max_age = ENPU_CONFIG_DEFAULT_LOG_MAX_AGE;
    config->log_console = ENPU_CONFIG_DEFAULT_LOG_CONSOLE;

    /* oversub_ratio 默认全 0（calloc 已清零，显式声明意图） */
    for (int i = 0; i < MAX_NPU_PER_NODE; i++) {
        config->oversub_ratio[i] = 0.0;
    }
    config->swap_pre_watermark = ENPU_CONFIG_DEFAULT_SWAP_PRE_WATERMARK;

    errno_t ret = strncpy_s(config->share_strategy, MAX_NAME_LEN, ENPU_CONFIG_DEFAULT_SHARE_STRATEGY,
                            strnlen(ENPU_CONFIG_DEFAULT_SHARE_STRATEGY, MAX_NAME_LEN - 1));
    if (ret != 0) {
        LOG_ERROR("[CONFIG] strncpy_s share_strategy failed, ret=%d", ret);
    }
}

/* 解析 oversub-ratio CSV 字符串，按 phy_id 顺序填入 config->oversub_ratio[] */
static void parse_oversub_ratio_csv(const char *val, enpu_config_t *config)
{
    if (val == NULL || config == NULL) {
        return;
    }

    /* 工作副本（strtok_r 会修改原串） */
    char buf[512];
    errno_t ret = strncpy_s(buf, sizeof(buf), val, strnlen(val, sizeof(buf) - 1));
    if (ret != 0) {
        LOG_ERROR("[CONFIG] strncpy_s buf failed, ret=%d", ret);
        return;
    }

    /* 第一遍：按 ',' 分割，记录所有 token */
    double values[MAX_NPU_PER_NODE];
    int count = 0;
    char *save = NULL;
    char *tok = strtok_r(buf, ",", &save);
    while (tok != NULL && count < MAX_NPU_PER_NODE) {
        /* trim 前导空格 */
        while (*tok == ' ' || *tok == '\t') {
            tok++;
        }
        values[count++] = strtod(tok, NULL);
        tok = strtok_r(NULL, ",", &save);
    }

    if (count == 0) {
        return;
    }

    /* 所有 die 都填这个值 */
    if (count == 1) {
        for (int i = 0; i < MAX_NPU_PER_NODE; i++) {
            config->oversub_ratio[i] = values[0];
        }
        return;
    }

    /* 按下标填，剩余 die 保持 0 */
    for (int i = 0; i < count && i < MAX_NPU_PER_NODE; i++) {
        config->oversub_ratio[i] = values[i];
    }
}

static bool parse_key_value(const char *key, const char *val, enpu_config_t *config)
{
    if (strcmp(key, "socket_path") == 0) {
        copy_config_str(config->socket_path, MAX_PATH_LEN, val);
    } else if (strcmp(key, "log_level") == 0) {
        copy_config_str(config->log_level, MAX_NAME_LEN, val);
    } else if (strcmp(key, "log_dir") == 0) {
        copy_config_str(config->log_dir, MAX_PATH_LEN, val);
    } else if (strcmp(key, "log_max_size") == 0) {
        config->log_max_size = atoi(val);
    } else if (strcmp(key, "log_max_backups") == 0) {
        config->log_max_backups = atoi(val);
    } else if (strcmp(key, "log_max_age") == 0) {
        config->log_max_age = atoi(val);
    } else if (strcmp(key, "log_console") == 0) {
        config->log_console = atoi(val);
    } else if (strcmp(key, "oversub-ratio") == 0) {
        parse_oversub_ratio_csv(val, config);
    } else if (strcmp(key, "swap-pre-watermark") == 0) {
        config->swap_pre_watermark = atoi(val);
    } else if (strcmp(key, "share-strategy") == 0) {
        copy_config_str(config->share_strategy, MAX_NAME_LEN, val);
    } else {
        return false;
    }
    return true;
}

/* 去除字符串首尾空白（空格、制表符、换行符） */
static void trim_whitespace(char *str, size_t buf_size)
{
    char *start = str;
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') {
        start++;
    }

    char *end = start + strlen(start);
    while (end > start && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\n' || end[-1] == '\r')) {
        end--;
    }

    if (start != str) {
        size_t copy_len = (size_t)(end - start) + 1;
        if (memmove_s(str, buf_size, start, copy_len) != 0) {
            LOG_ERROR("[CONFIG] trim_whitespace: memmove_s failed");
        }
    } else {
        *end = '\0';
    }
}

static bool load_from_file(const char *path, enpu_config_t *config)
{
    if (path == NULL) {
        path = ENPU_CONFIG_DEFAULT_PATH;
    }

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return true;
    }

    char line[256];
    size_t line_number = 0;
    bool success = true;
    while (fgets(line, sizeof(line), fp) != NULL) {
        line_number++;
        trim_whitespace(line, sizeof(line));
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        char *eq = strchr(line, '=');
        if (eq == NULL) {
            continue;
        }

        *eq = '\0';
        char *val = eq + 1;

        /* 剥内联注释：'#' 及其后的内容不参与解析 */
        char *comment = strchr(val, '#');
        if (comment != NULL) {
            *comment = '\0';
        }

        trim_whitespace(line, sizeof(line));
        trim_whitespace(val, sizeof(line) - (size_t)(val - line));

        if (!parse_key_value(line, val, config)) {
            (void)fprintf(stderr, "[ERROR] Unknown config key '%s' at %s:%zu\n", line, path, line_number);
            success = false;
            break;
        }
    }

    if (ferror(fp) != 0) {
        LOG_ERROR("[CONFIG] Failed to read config file: %s", path);
        success = false;
    }

    if (fclose(fp) != 0) {
        LOG_ERROR("[CONFIG] Failed to close config file: %s", path);
    }
    return success;
}

static void load_from_env(enpu_config_t *config)
{
    char *val = getenv("ENPU_SOCKET_PATH");
    if (val) {
        copy_config_str(config->socket_path, MAX_PATH_LEN, val);
    }

    val = getenv("ENPU_LOG_LEVEL");
    if (val) {
        copy_config_str(config->log_level, MAX_NAME_LEN, val);
    }

    val = getenv("ENPU_LOG_DIR");
    if (val) {
        copy_config_str(config->log_dir, MAX_PATH_LEN, val);
    }

    val = getenv("ENPU_LOG_MAX_SIZE");
    if (val) {
        config->log_max_size = atoi(val);
    }

    val = getenv("ENPU_LOG_MAX_BACKUPS");
    if (val) {
        config->log_max_backups = atoi(val);
    }

    val = getenv("ENPU_LOG_MAX_AGE");
    if (val) {
        config->log_max_age = atoi(val);
    }

    val = getenv("ENPU_LOG_CONSOLE");
    if (val) {
        config->log_console = atoi(val);
    }

    val = getenv("ENPU_OVERSUB_RATIO");
    if (val) {
        parse_oversub_ratio_csv(val, config);
    }

    val = getenv("ENPU_SWAP_PRE_WATERMARK");
    if (val) {
        config->swap_pre_watermark = atoi(val);
    }

    val = getenv("ENPU_SHARE_STRATEGY");
    if (val) {
        copy_config_str(config->share_strategy, MAX_NAME_LEN, val);
    }
}

enpu_config_t *enpu_config_load(const char *config_path)
{
    enpu_config_t *config = (enpu_config_t *)calloc(1, sizeof(enpu_config_t));
    if (config == NULL) {
        return NULL;
    }

    init_defaults(config);
    if (!load_from_file(config_path, config)) {
        free(config);
        return NULL;
    }
    load_from_env(config);

    return config;
}

int enpu_config_destroy(enpu_config_t *config)
{
    if (config == NULL) {
        return ENPU_INVALID_PARAM;
    }

    free(config);
    return ENPU_SUCCESS;
}

static void fsync_parent_dir(const char *path)
{
    char dir[MAX_PATH_LEN];
    snprintf_s(dir, sizeof(dir), (sizeof(dir)) - 1, "%s", path);
    char *slash = strrchr(dir, '/');
    if (slash == NULL) {
        return;
    }
    if (slash == dir) {
        slash[1] = '\0'; /* 根目录 "/" */
    } else {
        *slash = '\0';
    }

    int dir_fd = open(dir, O_RDONLY
#ifdef O_DIRECTORY
                               | O_DIRECTORY
#endif
    );
    if (dir_fd < 0) {
        return;
    }
    (void)fsync(dir_fd);
    close(dir_fd);
}

static void format_ratio(char *buf, size_t len, double v)
{
    snprintf_s(buf, len, (len)-1, "%.6g", v);
    if (strtod(buf, NULL) != v) {
        snprintf_s(buf, len, (len)-1, "%.17g", v);
    }
}

int enpu_config_write_oversub_ratio(const char *path, const double per_die_ratio[MAX_NPU_PER_NODE], int phy_id_span)
{
    if (path == NULL || per_die_ratio == NULL || phy_id_span <= 0 || phy_id_span > MAX_NPU_PER_NODE) {
        return ENPU_INVALID_PARAM;
    }

    /* 单值会广播到全部 MAX_NPU_PER_NODE 项。少于容量上限时必须写 CSV，
     * 即使比例相同，也不能在重启时为不存在的设备启用超分。
     * 仅有 phy0 时补一个零占位，避免单项 CSV 被解析为广播。 */
    char value[512];
    int write_span = (phy_id_span == 1) ? 2 : phy_id_span;
    bool all_equal = true;
    for (int i = 1; i < phy_id_span; i++) {
        if (per_die_ratio[i] != per_die_ratio[0]) {
            all_equal = false;
            break;
        }
    }
    if (all_equal && phy_id_span == MAX_NPU_PER_NODE) {
        format_ratio(value, sizeof(value), per_die_ratio[0]);
    } else {
        int off = 0;
        for (int i = 0; i < write_span && off < (int)sizeof(value) - 32; i++) {
            char item[32];
            format_ratio(item, sizeof(item), (i < phy_id_span) ? per_die_ratio[i] : 0.0);
            off += snprintf_s(value + off, sizeof(value) - off, (sizeof(value) - off) - 1, "%s%s", (i > 0) ? "," : "",
                              item);
        }
    }

    char tmp_path[MAX_PATH_LEN];
    char bak_path[MAX_PATH_LEN];
    snprintf_s(tmp_path, sizeof(tmp_path), (sizeof(tmp_path)) - 1, "%s.tmp", path);
    snprintf_s(bak_path, sizeof(bak_path), (sizeof(bak_path)) - 1, "%s.bak", path);

    FILE *fp = fopen(path, "r");
    bool orig_exists = (fp != NULL);

    FILE *tmp = fopen(tmp_path, "w");
    if (tmp == NULL) {
        if (fp != NULL)
            fclose(fp);
        return ENPU_FAIL;
    }

    bool replaced = false;
    bool needs_newline = false;
    if (fp != NULL) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            char *s = line;
            while (*s == ' ' || *s == '\t')
                s++;
            if (strncmp(s, "oversub-ratio", 13) == 0 &&
                (s[13] == '=' || s[13] == ' ' || s[13] == '\t' || s[13] == '\n' || s[13] == '\r' || s[13] == '\0')) {
                fprintf(tmp, "oversub-ratio=%s\n", value);
                replaced = true;
            } else {
                fputs(line, tmp);
                size_t len = strlen(line);
                needs_newline = len > 0 && line[len - 1] != '\n';
            }
        }
        fclose(fp);
    }

    if (!replaced) {
        if (needs_newline)
            fputc('\n', tmp);
        fprintf(tmp, "oversub-ratio=%s\n", value);
    }

    if (fflush(tmp) != 0 || fsync(fileno(tmp)) != 0) {
        fclose(tmp);
        unlink(tmp_path);
        return ENPU_FAIL;
    }
    fclose(tmp);

    /* 写前备份 .bak */
    if (orig_exists) {
        FILE *src = fopen(path, "r");
        FILE *dst = fopen(bak_path, "w");
        bool backup_ok = (src != NULL && dst != NULL);
        if (src != NULL && dst != NULL) {
            char buf[4096];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
                if (fwrite(buf, 1, n, dst) != n) {
                    backup_ok = false;
                    break;
                }
            }
            if (ferror(src))
                backup_ok = false;
            if (fflush(dst) != 0 || fsync(fileno(dst)) != 0)
                backup_ok = false;
        }
        if (src != NULL && fclose(src) != 0)
            backup_ok = false;
        if (dst != NULL && fclose(dst) != 0)
            backup_ok = false;
        if (!backup_ok) {
            unlink(tmp_path);
            return ENPU_FAIL;
        }
    }

    if (rename(tmp_path, path) != 0) {
        unlink(tmp_path);
        return ENPU_FAIL;
    }
    fsync_parent_dir(path);

    return ENPU_SUCCESS;
}
