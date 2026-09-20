/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 *
 * npu_allocator_internal.h - STRUCTURE DEFINITION for internal modules
 *
 * This header contains the npu_allocator_t structure definition.
 * It is ONLY for internal modules that need direct access to allocator members.
 * External callers should use opaque pointer via npu_allocator.h.
 *
 * Allowed users: npu_allocator.c, rollback_log.c
 * Location: core/internal/
 */

#ifndef __NPU_ALLOCATOR_INTERNAL_STRUCT_H__
#define __NPU_ALLOCATOR_INTERNAL_STRUCT_H__

#include <pthread.h>
#include "../include/enpu_manager.h"

typedef struct evaluator_registry evaluator_registry_t;
typedef struct rollback_log rollback_log_t;
typedef struct npu_tree npu_tree_t;
typedef struct config_manager config_manager_t;
typedef struct shm_manager shm_manager_t;
typedef struct dcmi_stub_config dcmi_stub_config_t;

typedef struct npu_allocator {
    pthread_mutex_t lock;
    npu_tree_t *tree;
    config_manager_t *config_mgr;
    shm_manager_t *shm_mgr;
    allocation_registry_t *registry;
    evaluator_registry_t *eval_registry;
    rollback_log_t *rollback_log;
    struct evaluator **evaluators;
    int evaluator_count;
    char checkpoint_path[MAX_PATH_LEN];
    char config_base_path[MAX_PATH_LEN];
    char share_strategy[MAX_NAME_LEN];
    char evaluator_name[MAX_NAME_LEN];
    bool use_dcmi_stub;
    dcmi_stub_config_t dcmi_stub;
    double oversub_ratio[MAX_NPU_PER_NODE];
} npu_allocator_t;

#endif