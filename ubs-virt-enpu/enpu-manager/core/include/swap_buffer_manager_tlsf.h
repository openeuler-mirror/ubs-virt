/*
* Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
* ubs-virt-ovs is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v2 for more details.
*/

#ifndef __SWAP_BUFFER_MANAGER_TLSF_H__
#define __SWAP_BUFFER_MANAGER_TLSF_H__

#include <stdint.h>
#include "allocation.h"
#include "common.h"

#define SWAP_BUFFER_POOL_SIZE (1ULL * 1024ULL * 1024ULL * 1024ULL)

typedef struct tlsf_status {
    uint64_t total;
    uint64_t used;
    uint64_t free;
    uint64_t overhead;
    int allocated_blocks;
    int free_blocks;
} tlsf_status_t;

typedef struct tlsf_pool tlsf_pool_t;

#if defined(__cplusplus)
extern "C" {
#endif

tlsf_pool_t *tlsf_create(uint64_t pool_size);
tlsf_pool_t *tlsf_create_with_shm(uint64_t pool_size);
int tlsf_destroy(tlsf_pool_t *pool);

int tlsf_malloc(tlsf_pool_t *pool, uint64_t size, const char *pod_uid, int vnpu_id, uint64_t *offset);
int tlsf_free(tlsf_pool_t *pool, uint64_t offset);
int tlsf_get_status(tlsf_pool_t *pool, tlsf_status_t *status);
int tlsf_recover(tlsf_pool_t *pool, allocation_t *allocs, int count);

void tlsf_debug_dump(tlsf_pool_t *pool);

#if defined(__cplusplus)
}
#endif

#endif