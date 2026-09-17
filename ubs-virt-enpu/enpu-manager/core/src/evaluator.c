/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A SPECIFIC PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include "evaluator.h"
#include <stdlib.h>
#include <string.h>

#define MAX_EVALUATORS 16

struct evaluator_registry {
    evaluator_t *evaluators[MAX_EVALUATORS];
    int count;
};

evaluator_registry_t *evaluator_registry_create(void)
{
    evaluator_registry_t *registry = (evaluator_registry_t *)calloc(1, sizeof(evaluator_registry_t));
    return registry;
}

void evaluator_registry_destroy(evaluator_registry_t *registry)
{
    if (registry == NULL) {
        return;
    }
    free(registry);
}

int evaluator_registry_register(evaluator_registry_t *registry, evaluator_t *eval)
{
    if (registry == NULL || eval == NULL || eval->name == NULL) {
        return ENPU_INVALID_PARAM;
    }

    if (registry->count >= MAX_EVALUATORS) {
        return ENPU_FAIL;
    }

    for (int i = 0; i < registry->count; i++) {
        if (strcmp(registry->evaluators[i]->name, eval->name) == 0) {
            return ENPU_FAIL;
        }
    }

    registry->evaluators[registry->count++] = eval;
    return ENPU_SUCCESS;
}

evaluator_t *evaluator_registry_get(evaluator_registry_t *registry, const char *name)
{
    if (registry == NULL || name == NULL) {
        return NULL;
    }

    for (int i = 0; i < registry->count; i++) {
        if (strcmp(registry->evaluators[i]->name, name) == 0) {
            return registry->evaluators[i];
        }
    }
    return NULL;
}

void evaluator_registry_clear(evaluator_registry_t *registry)
{
    if (registry == NULL) {
        return;
    }
    registry->count = 0;
}
