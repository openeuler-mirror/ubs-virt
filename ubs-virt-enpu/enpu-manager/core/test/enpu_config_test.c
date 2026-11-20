/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 */
#include "enpu_config.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void write_config(const char *path, const char *content)
{
    FILE *file = fopen(path, "w");
    assert(file != NULL);
    assert(fputs(content, file) >= 0);
    assert(fclose(file) == 0);
}

int main(void)
{
    char path[] = "/tmp/enpu-manager-config-test-XXXXXX";
    int fd = mkstemp(path);
    assert(fd >= 0);
    assert(close(fd) == 0);

    write_config(path, "  # config_dir=/tmp/enpu\n\nsocket_path = /tmp/enpu-manager-test.sock\n");
    enpu_config_t *config = enpu_config_load(path);
    assert(config != NULL);
    assert(strcmp(config->socket_path, "/tmp/enpu-manager-test.sock") == 0);
    assert(enpu_config_destroy(config) == ENPU_SUCCESS);

    write_config(path, "socket_path\t=\t/tmp/enpu-manager-tab-test.sock\n");
    config = enpu_config_load(path);
    assert(config != NULL);
    assert(strcmp(config->socket_path, "/tmp/enpu-manager-tab-test.sock") == 0);
    assert(enpu_config_destroy(config) == ENPU_SUCCESS);

    write_config(path, "config_dir=/tmp/enpu\n");
    assert(enpu_config_load(path) == NULL);

    write_config(path, "# keep this comment\nshare-strategy = compact # old value\nlog_level=info\n");
    assert(enpu_config_update_value(path, "share-strategy", "anti-fragment") == ENPU_SUCCESS);
    assert(enpu_config_update_value(path, "swap-pre-watermark", "85") == ENPU_SUCCESS);
    assert(enpu_config_update_value(path, "unknown", "1") == ENPU_INVALID_PARAM);
    config = enpu_config_load(path);
    assert(config != NULL);
    assert(strcmp(config->share_strategy, "anti-fragment") == 0);
    assert(config->swap_pre_watermark == 85);
    assert(strcmp(config->log_level, "info") == 0);
    assert(enpu_config_destroy(config) == ENPU_SUCCESS);
    FILE *updated = fopen(path, "r");
    assert(updated != NULL);
    char contents[256] = {0};
    assert(fread(contents, 1, sizeof(contents) - 1, updated) > 0);
    assert(strstr(contents, "# keep this comment") != NULL);
    assert(strstr(contents, "share-strategy=anti-fragment") != NULL);
    assert(strstr(contents, "swap-pre-watermark=85") != NULL);
    assert(fclose(updated) == 0);

    assert(unlink(path) == 0);
    return 0;
}
