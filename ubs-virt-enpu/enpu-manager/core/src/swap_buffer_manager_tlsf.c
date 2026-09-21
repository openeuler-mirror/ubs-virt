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

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "allocation.h"
#include "log.h"
#include "securec.h"
/* SWAP_BUFFER_SHM_NAME 统一定义在 swap_buffer_manager.h，此处不再重复定义，
 * 避免两处字面量各自演进后指向不同的 shm 对象 */
#include "swap_buffer_manager.h"
#include "swap_buffer_manager_tlsf.h"

#define TLSF_FL_INDEX_MAX 22
#define TLSF_SL_INDEX_COUNT 16
#define TLSF_SL_INDEX_COUNT_LOG2 4
#define TLSF_FL_INDEX_MIN 6
#define TLSF_SL_INDEX_MAX (TLSF_SL_INDEX_COUNT - 1)
#define TLSF_ALIGN_SIZE_LOG2 5
#define TLSF_ALIGN_SIZE (1ULL << TLSF_ALIGN_SIZE_LOG2)
#define TLSF_ALIGN_MASK (TLSF_ALIGN_SIZE - 1)

#define TLSF_BLOCK_HEADER_SIZE 48
#define TLSF_BLOCK_HEADER_OVERHEAD TLSF_ALIGN_UP(TLSF_BLOCK_HEADER_SIZE)
#define TLSF_ALIGN_UP(size) (((size) + TLSF_ALIGN_MASK) & ~TLSF_ALIGN_MASK)
#define TLSF_ALIGN_DOWN(size) ((size) & ~TLSF_ALIGN_MASK)

#define TLSF_BLOCK_SIZE_MIN (TLSF_ALIGN_UP(TLSF_BLOCK_HEADER_OVERHEAD + TLSF_ALIGN_SIZE))
#define TLSF_BLOCK_SIZE_MAX (1ULL << (TLSF_FL_INDEX_MAX - 1))

#define SWAP_ALLOC_ALIGN_UP(size) (TLSF_ALIGN_UP(size) + TLSF_ALIGN_SIZE)

#define TLSF_GET_BLOCK_HEADER(ptr) ((tlsf_block_header_t *)((uintptr_t)(ptr)-TLSF_BLOCK_HEADER_OVERHEAD))
#define TLSF_GET_BLOCK_PAYLOAD(hdr) ((void *)((uintptr_t)(hdr) + TLSF_BLOCK_HEADER_OVERHEAD))
#define TLSF_GET_NEXT_PHYS_BLOCK(hdr, size) \
    ((tlsf_block_header_t *)((uintptr_t)(hdr) + TLSF_BLOCK_HEADER_OVERHEAD + (size)))

typedef struct tlsf_block_header {
    struct tlsf_block_header *prev_phys_block;
    struct tlsf_block_header *next_free_block;
    struct tlsf_block_header *prev_free_block;
    uint64_t block_size;
    uint64_t payload_offset;
    char pod_uid[MAX_UUID_LEN];
    int vnpu_id;
    uint32_t is_free;
    uint32_t is_last_block;
} tlsf_block_header_t;

typedef struct tlsf_control {
    tlsf_block_header_t *null_block;
    uint64_t fl_bitmap;
    uint64_t sl_bitmap[TLSF_FL_INDEX_MAX];
    tlsf_block_header_t *blocks[TLSF_FL_INDEX_MAX][TLSF_SL_INDEX_COUNT];
} tlsf_control_t;

struct tlsf_pool {
    tlsf_control_t control;
    void *base_addr;
    uint64_t pool_size;
    uint64_t overhead_size;
    atomic_uint_fast64_t used_size;
    atomic_uint_fast64_t allocated_count;
    pthread_mutex_t lock;
    int shm_fd;
    bool is_initialized;
};

static inline int tlsf_ffs(uint64_t word)
{
    if (word == 0)
        return -1;
    int bit = 0;
    while ((word & (1ULL << bit)) == 0) {
        bit++;
    }
    return bit;
}

static inline int tlsf_fls(uint64_t word)
{
    if (word == 0)
        return -1;
    int bit = 63;
    while ((word & (1ULL << bit)) == 0) {
        bit--;
    }
    return bit;
}

static inline int tlsf_fls_sizet(uint64_t size)
{
    return tlsf_fls(size);
}

static void tlsf_mapping_insert(uint64_t size, int *fli, int *sli)
{
    if (size < TLSF_BLOCK_SIZE_MIN) {
        size = TLSF_BLOCK_SIZE_MIN;
    }

    int fl = tlsf_fls_sizet(size);
    int sl = (int)((size >> (fl - TLSF_SL_INDEX_COUNT_LOG2)) - (1ULL << TLSF_SL_INDEX_COUNT_LOG2));

    if (sl < 0) {
        fl--;
        sl = (int)((size >> (fl - TLSF_SL_INDEX_COUNT_LOG2)) - (1ULL << TLSF_SL_INDEX_COUNT_LOG2));
    }

    if (fl < TLSF_FL_INDEX_MIN) {
        fl = TLSF_FL_INDEX_MIN;
        sl = 0;
    }

    *fli = fl;
    *sli = sl;
}

static void tlsf_mapping_search(uint64_t size, int *fli, int *sli)
{
    int fl, sl;

    if (size >= TLSF_BLOCK_SIZE_MIN) {
        uint64_t round_size = TLSF_ALIGN_UP(size);
        tlsf_mapping_insert(round_size, &fl, &sl);
        if (sl > 0) {
            sl--;
        }
    } else {
        fl = TLSF_FL_INDEX_MIN;
        sl = 0;
    }

    *fli = fl;
    *sli = sl;
}

static inline void tlsf_set_bit(uint64_t *bitmap, int bit)
{
    *bitmap |= (1ULL << bit);
}

static inline void tlsf_clear_bit(uint64_t *bitmap, int bit)
{
    *bitmap &= ~(1ULL << bit);
}

static inline int tlsf_test_bit(uint64_t bitmap, int bit)
{
    return (bitmap & (1ULL << bit)) != 0;
}

static tlsf_block_header_t *tlsf_find_suitable_block(tlsf_pool_t *pool, int fl, int sl)
{
    uint64_t sl_map = pool->control.sl_bitmap[fl] & (~0ULL << sl);

    if (sl_map == 0) {
        uint64_t fl_map = pool->control.fl_bitmap & (~0ULL << (fl + 1));
        if (fl_map == 0)
            return NULL;

        fl = tlsf_ffs(fl_map);
        sl_map = pool->control.sl_bitmap[fl];
    }

    sl = tlsf_ffs(sl_map);
    return pool->control.blocks[fl][sl];
}

static void tlsf_remove_free_block(tlsf_pool_t *pool, tlsf_block_header_t *block, int fl, int sl)
{
    tlsf_block_header_t *prev_free = block->prev_free_block;
    tlsf_block_header_t *next_free = block->next_free_block;

    if (next_free) {
        next_free->prev_free_block = prev_free;
    }
    if (prev_free) {
        prev_free->next_free_block = next_free;
    }

    if (pool->control.blocks[fl][sl] == block) {
        pool->control.blocks[fl][sl] = next_free;
    }

    if (pool->control.blocks[fl][sl] == NULL) {
        tlsf_clear_bit(&pool->control.sl_bitmap[fl], sl);
        if (pool->control.sl_bitmap[fl] == 0) {
            tlsf_clear_bit(&pool->control.fl_bitmap, fl);
        }
    }
}

static void tlsf_insert_free_block(tlsf_pool_t *pool, tlsf_block_header_t *block, int fl, int sl)
{
    tlsf_block_header_t *current = pool->control.blocks[fl][sl];

    block->next_free_block = current;
    block->prev_free_block = NULL;

    if (current) {
        current->prev_free_block = block;
    }

    pool->control.blocks[fl][sl] = block;

    tlsf_set_bit(&pool->control.sl_bitmap[fl], sl);
    tlsf_set_bit(&pool->control.fl_bitmap, fl);
}

static void tlsf_block_insert(tlsf_pool_t *pool, tlsf_block_header_t *block)
{
    int fl, sl;
    tlsf_mapping_insert(block->block_size, &fl, &sl);
    tlsf_insert_free_block(pool, block, fl, sl);
}

static void tlsf_block_remove(tlsf_pool_t *pool, tlsf_block_header_t *block)
{
    int fl, sl;
    tlsf_mapping_insert(block->block_size, &fl, &sl);
    tlsf_remove_free_block(pool, block, fl, sl);
}

static tlsf_block_header_t *tlsf_split_block(tlsf_pool_t *pool, tlsf_block_header_t *block, uint64_t size)
{
    uint64_t remain = block->block_size - size;

    if (remain < TLSF_BLOCK_SIZE_MIN) {
        return NULL;
    }

    block->block_size = size;

    tlsf_block_header_t *remaining_block = TLSF_GET_NEXT_PHYS_BLOCK(block, size);
    remaining_block->block_size = remain;
    remaining_block->is_free = 1;
    remaining_block->is_last_block = block->is_last_block;
    remaining_block->prev_phys_block = block;
    remaining_block->payload_offset = block->payload_offset + TLSF_BLOCK_HEADER_OVERHEAD + size;
    remaining_block->next_free_block = NULL;
    remaining_block->prev_free_block = NULL;
    memset_s(remaining_block->pod_uid, MAX_UUID_LEN, 0, MAX_UUID_LEN);
    remaining_block->vnpu_id = -1;

    if (!block->is_last_block) {
        tlsf_block_header_t *next_block = TLSF_GET_NEXT_PHYS_BLOCK(remaining_block, remain);
        next_block->prev_phys_block = remaining_block;
    }

    block->is_last_block = 0;

    tlsf_block_insert(pool, remaining_block);

    return remaining_block;
}

static tlsf_block_header_t *tlsf_merge_prev_block(tlsf_pool_t *pool, tlsf_block_header_t *block)
{
    if (block->prev_phys_block && block->prev_phys_block->is_free) {
        tlsf_block_header_t *prev = block->prev_phys_block;

        tlsf_block_remove(pool, prev);

        prev->block_size += TLSF_BLOCK_HEADER_OVERHEAD + block->block_size;
        prev->is_last_block = block->is_last_block;

        if (!block->is_last_block) {
            tlsf_block_header_t *next = TLSF_GET_NEXT_PHYS_BLOCK(prev, prev->block_size);
            next->prev_phys_block = prev;
        }

        return prev;
    }

    return block;
}

static tlsf_block_header_t *tlsf_merge_next_block(tlsf_pool_t *pool, tlsf_block_header_t *block)
{
    if (!block->is_last_block) {
        tlsf_block_header_t *next = TLSF_GET_NEXT_PHYS_BLOCK(block, block->block_size);
        if (next->is_free) {
            tlsf_block_remove(pool, next);

            block->block_size += TLSF_BLOCK_HEADER_OVERHEAD + next->block_size;
            block->is_last_block = next->is_last_block;

            if (!next->is_last_block) {
                tlsf_block_header_t *next_next = TLSF_GET_NEXT_PHYS_BLOCK(block, block->block_size);
                next_next->prev_phys_block = block;
            }
        }
    }

    return block;
}

static void tlsf_merge_free_block(tlsf_pool_t *pool, tlsf_block_header_t *block)
{
    block = tlsf_merge_prev_block(pool, block);
    block = tlsf_merge_next_block(pool, block);

    tlsf_block_insert(pool, block);
}

static tlsf_block_header_t *tlsf_locate_block_by_offset(tlsf_pool_t *pool, uint64_t offset)
{
    tlsf_block_header_t *block = pool->control.null_block;

    while (block) {
        if (block->payload_offset == offset) {
            return block;
        }

        if (block->is_last_block)
            break;

        block = TLSF_GET_NEXT_PHYS_BLOCK(block, block->block_size);
    }

    return NULL;
}

static void tlsf_init_control(tlsf_pool_t *pool)
{
    pool->control.null_block = NULL;
    pool->control.fl_bitmap = 0;

    for (int i = 0; i < TLSF_FL_INDEX_MAX; i++) {
        pool->control.sl_bitmap[i] = 0;
        for (int j = 0; j < TLSF_SL_INDEX_COUNT; j++) {
            pool->control.blocks[i][j] = NULL;
        }
    }
}

static void tlsf_add_initial_block(tlsf_pool_t *pool, uint64_t size)
{
    uint64_t block_size = TLSF_ALIGN_DOWN(size - TLSF_BLOCK_HEADER_OVERHEAD);

    tlsf_block_header_t *block = (tlsf_block_header_t *)pool->base_addr;
    block->prev_phys_block = NULL;
    block->next_free_block = NULL;
    block->prev_free_block = NULL;
    block->block_size = block_size;
    block->payload_offset = TLSF_BLOCK_HEADER_OVERHEAD;
    block->is_free = 1;
    block->is_last_block = 1;
    memset_s(block->pod_uid, MAX_UUID_LEN, 0, MAX_UUID_LEN);
    block->vnpu_id = -1;

    pool->control.null_block = block;

    tlsf_block_insert(pool, block);
}

tlsf_pool_t *tlsf_create(uint64_t pool_size)
{
    if (pool_size < TLSF_BLOCK_SIZE_MIN * 2) {
        LOG_ERROR("[TLSF] pool_size too small, min=%lu", TLSF_BLOCK_SIZE_MIN * 2);
        return NULL;
    }

    tlsf_pool_t *pool = (tlsf_pool_t *)calloc(1, sizeof(tlsf_pool_t));
    if (pool == NULL) {
        LOG_ERROR("[TLSF] calloc pool failed");
        return NULL;
    }

    pool->base_addr = malloc(pool_size);
    if (pool->base_addr == NULL) {
        LOG_ERROR("[TLSF] malloc pool buffer failed");
        free(pool);
        return NULL;
    }

    pool->pool_size = pool_size;
    pool->overhead_size = TLSF_BLOCK_HEADER_OVERHEAD;
    atomic_store(&pool->used_size, 0);
    atomic_store(&pool->allocated_count, 0);
    pool->shm_fd = -1;
    pool->is_initialized = false;

    if (pthread_mutex_init(&pool->lock, NULL) != 0) {
        LOG_ERROR("[TLSF] pthread_mutex_init failed");
        free(pool->base_addr);
        free(pool);
        return NULL;
    }

    tlsf_init_control(pool);
    tlsf_add_initial_block(pool, pool_size);

    pool->is_initialized = true;

    LOG_INFO("[TLSF] pool created: size=%lu, base_addr=%p", pool_size, pool->base_addr);

    return pool;
}

tlsf_pool_t *tlsf_create_with_shm(uint64_t pool_size)
{
    /* shm 池的大小必须与运行侧及其它进程约定一致，固定取 SWAP_BUFFER_POOL_SIZE，
     * 入参 pool_size 不生效，保留仅为兼容既有调用方 */
    (void)pool_size;
    const uint64_t shm_pool_size = SWAP_BUFFER_POOL_SIZE;

    if (shm_pool_size < TLSF_BLOCK_SIZE_MIN * 2) {
        LOG_ERROR("[TLSF] pool_size too small, min=%lu", TLSF_BLOCK_SIZE_MIN * 2);
        return NULL;
    }

    tlsf_pool_t *pool = (tlsf_pool_t *)calloc(1, sizeof(tlsf_pool_t));
    if (pool == NULL) {
        LOG_ERROR("[TLSF] calloc failed");
        return NULL;
    }

    pool->shm_fd = swap_buffer_shm_open();
    if (pool->shm_fd < 0) {
        LOG_ERROR("[TLSF] shm_open failed: %s", strerror(errno));
        free(pool);
        return NULL;
    }

    if (swap_buffer_shm_truncate(pool->shm_fd, shm_pool_size) != 0) {
        LOG_ERROR("[TLSF] ftruncate failed: %s", strerror(errno));
        close(pool->shm_fd);
        swap_buffer_shm_unlink();
        free(pool);
        return NULL;
    }

    pool->base_addr = mmap(NULL, shm_pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, pool->shm_fd, 0);
    if (pool->base_addr == MAP_FAILED) {
        LOG_ERROR("[TLSF] mmap failed: %s", strerror(errno));
        close(pool->shm_fd);
        swap_buffer_shm_unlink();
        free(pool);
        return NULL;
    }

    pool->pool_size = shm_pool_size;
    pool->overhead_size = TLSF_BLOCK_HEADER_OVERHEAD;
    atomic_store(&pool->used_size, 0);
    atomic_store(&pool->allocated_count, 0);
    pool->is_initialized = false;

    if (pthread_mutex_init(&pool->lock, NULL) != 0) {
        LOG_ERROR("[TLSF] pthread_mutex_init failed");
        munmap(pool->base_addr, shm_pool_size);
        close(pool->shm_fd);
        swap_buffer_shm_unlink();
        free(pool);
        return NULL;
    }

    tlsf_init_control(pool);
    tlsf_add_initial_block(pool, shm_pool_size);

    pool->is_initialized = true;

    LOG_INFO("[TLSF] shm pool created: name=%s, size=%lu, base_addr=%p, fd=%d", SWAP_BUFFER_SHM_NAME, shm_pool_size,
             pool->base_addr, pool->shm_fd);

    return pool;
}

int tlsf_destroy(tlsf_pool_t *pool)
{
    if (pool == NULL) {
        return ENPU_INVALID_PARAM;
    }

    pthread_mutex_lock(&pool->lock);

    if (pool->shm_fd >= 0) {
        if (pool->base_addr && pool->base_addr != MAP_FAILED) {
            munmap(pool->base_addr, pool->pool_size);
        }
        close(pool->shm_fd);
        swap_buffer_shm_unlink();
        pool->shm_fd = -1;
    } else if (pool->base_addr) {
        free(pool->base_addr);
    }

    pool->base_addr = NULL;
    pool->is_initialized = false;

    pthread_mutex_unlock(&pool->lock);
    pthread_mutex_destroy(&pool->lock);

    free(pool);

    LOG_INFO("[TLSF] pool destroyed");

    return ENPU_SUCCESS;
}

int tlsf_malloc(tlsf_pool_t *pool, uint64_t size, const char *pod_uid, int vnpu_id, uint64_t *offset)
{
    if (pool == NULL || size == 0 || pod_uid == NULL || vnpu_id < 0 || offset == NULL) {
        return ENPU_INVALID_PARAM;
    }

    if (!pool->is_initialized) {
        return ENPU_FAIL;
    }

    uint64_t adjust_size = SWAP_ALLOC_ALIGN_UP(size);

    if (adjust_size < TLSF_BLOCK_SIZE_MIN) {
        adjust_size = TLSF_BLOCK_SIZE_MIN;
    }

    int fl, sl;
    tlsf_mapping_search(adjust_size, &fl, &sl);

    pthread_mutex_lock(&pool->lock);

    tlsf_block_header_t *block = tlsf_find_suitable_block(pool, fl, sl);
    if (block == NULL) {
        pthread_mutex_unlock(&pool->lock);
        LOG_ERROR("[TLSF] malloc failed: no suitable block for size=%lu (request=%lu)", adjust_size, size);
        return ENPU_NO_RESOURCE;
    }

    tlsf_block_remove(pool, block);

    if (block->block_size >= adjust_size + TLSF_BLOCK_SIZE_MIN) {
        tlsf_split_block(pool, block, adjust_size);
    }

    block->is_free = 0;
    if (snprintf_s(block->pod_uid, MAX_UUID_LEN, MAX_UUID_LEN - 1, "%s", pod_uid) < 0) {
        pthread_mutex_unlock(&pool->lock);
        LOG_ERROR("[TLSF] malloc failed: format pod_uid error, offset=%lu", block->payload_offset);
        return ENPU_FAIL;
    }
    block->vnpu_id = vnpu_id;

    *offset = block->payload_offset;

    atomic_fetch_add(&pool->used_size, block->block_size);
    atomic_fetch_add(&pool->allocated_count, 1);

    pthread_mutex_unlock(&pool->lock);

    LOG_INFO("[TLSF] malloc: request=%lu, allocated=%lu, offset=%lu, pod=%s, vnpu_id=%d", size, block->block_size,
             *offset, pod_uid, vnpu_id);

    return ENPU_SUCCESS;
}

int tlsf_free(tlsf_pool_t *pool, uint64_t offset)
{
    if (pool == NULL) {
        return ENPU_INVALID_PARAM;
    }

    if (!pool->is_initialized) {
        return ENPU_FAIL;
    }

    pthread_mutex_lock(&pool->lock);

    tlsf_block_header_t *block = tlsf_locate_block_by_offset(pool, offset);
    if (block == NULL) {
        pthread_mutex_unlock(&pool->lock);
        LOG_ERROR("[TLSF] free failed: block not found for offset=%lu", offset);
        return ENPU_NOT_FOUND;
    }

    if (block->is_free) {
        pthread_mutex_unlock(&pool->lock);
        LOG_ERROR("[TLSF] free failed: block already free, offset=%lu", offset);
        return ENPU_INVALID_PARAM;
    }

    uint64_t freed_size = block->block_size;

    block->is_free = 1;
    memset_s(block->pod_uid, MAX_UUID_LEN, 0, MAX_UUID_LEN);
    block->vnpu_id = -1;

    tlsf_merge_free_block(pool, block);

    atomic_fetch_sub(&pool->used_size, freed_size);
    atomic_fetch_sub(&pool->allocated_count, 1);

    pthread_mutex_unlock(&pool->lock);

    LOG_INFO("[TLSF] free: offset=%lu, size=%lu", offset, freed_size);

    return ENPU_SUCCESS;
}

int tlsf_get_status(tlsf_pool_t *pool, tlsf_status_t *status)
{
    if (pool == NULL || status == NULL) {
        return ENPU_INVALID_PARAM;
    }

    pthread_mutex_lock(&pool->lock);

    status->total = pool->pool_size;
    status->used = atomic_load(&pool->used_size);
    status->overhead = pool->overhead_size;
    status->free = pool->pool_size - status->used - status->overhead;
    status->allocated_blocks = (int)atomic_load(&pool->allocated_count);

    int free_count = 0;
    tlsf_block_header_t *block = pool->control.null_block;
    while (block) {
        if (block->is_free)
            free_count++;
        if (block->is_last_block)
            break;
        block = TLSF_GET_NEXT_PHYS_BLOCK(block, block->block_size);
    }
    status->free_blocks = free_count;

    pthread_mutex_unlock(&pool->lock);

    return ENPU_SUCCESS;
}

int tlsf_recover(tlsf_pool_t *pool, allocation_t *allocs, int count)
{
    if (pool == NULL) {
        return ENPU_INVALID_PARAM;
    }

    if (!pool->is_initialized) {
        return ENPU_FAIL;
    }

    if (allocs == NULL || count <= 0) {
        return ENPU_SUCCESS;
    }

    pthread_mutex_lock(&pool->lock);

    for (int i = 0; i < count; i++) {
        allocation_t *alloc = &allocs[i];

        if (!alloc->swapped || alloc->swap_size == 0)
            continue;

        /* swap_size 存的是申请时的原始大小，分配路径按 SWAP_ALLOC_ALIGN_UP 换算，
         * 恢复路径必须用同一换算，否则同一笔申请在恢复后尺寸类不一致 */
        uint64_t adjust_size = SWAP_ALLOC_ALIGN_UP(alloc->swap_size);
        if (adjust_size < TLSF_BLOCK_SIZE_MIN) {
            adjust_size = TLSF_BLOCK_SIZE_MIN;
        }

        int fl, sl;
        tlsf_mapping_search(adjust_size, &fl, &sl);

        tlsf_block_header_t *free_block = tlsf_find_suitable_block(pool, fl, sl);
        if (free_block == NULL) {
            pthread_mutex_unlock(&pool->lock);
            LOG_ERROR("[TLSF] recover failed: no block for alloc offset=%lu", alloc->swap_offset);
            return ENPU_NO_RESOURCE;
        }

        tlsf_block_remove(pool, free_block);

        if (free_block->block_size >= adjust_size + TLSF_BLOCK_SIZE_MIN) {
            tlsf_split_block(pool, free_block, adjust_size);
        }

        free_block->payload_offset = alloc->swap_offset;
        /* 不再强改 block_size：真正 split 过时 tlsf_split_block 已按 adjust_size 写好，这行是多余的；
         * 未 split 时原块比 adjust_size 大，强改会让后续按 block_size 找下一物理块时错位。
         * 保留原尺寸后 block_size 不再等于 adjust_size，因此下面的记账必须跟着用 block_size
         * （与 tlsf_malloc 的记账口径一致），否则 tlsf_free 按 block->block_size 扣减会对不上 */
        free_block->is_free = 0;
        if (snprintf_s(free_block->pod_uid, MAX_UUID_LEN, MAX_UUID_LEN - 1, "%s", alloc->pod_uid) < 0) {
            pthread_mutex_unlock(&pool->lock);
            LOG_ERROR("[TLSF] recover failed: format pod_uid error, offset=%lu", alloc->swap_offset);
            return ENPU_FAIL;
        }
        free_block->vnpu_id = alloc->vnpu_id;

        atomic_fetch_add(&pool->used_size, free_block->block_size);
        atomic_fetch_add(&pool->allocated_count, 1);
    }

    pthread_mutex_unlock(&pool->lock);

    LOG_INFO("[TLSF] recover: recovered %d allocations", count);

    return ENPU_SUCCESS;
}

void tlsf_debug_dump(tlsf_pool_t *pool)
{
    if (pool == NULL)
        return;

    pthread_mutex_lock(&pool->lock);

    printf("\n=== TLSF Pool Debug Dump ===\n");
    printf("Pool Info:\n");
    printf("  size: %lu bytes\n", pool->pool_size);
    printf("  base_addr: %p\n", pool->base_addr);
    printf("  used: %lu bytes\n", atomic_load(&pool->used_size));
    printf("  shm_fd: %d\n", pool->shm_fd);
    printf("\nBitmaps:\n");
    printf("  FL bitmap: 0x%lx\n", pool->control.fl_bitmap);

    printf("\nBlock List (physical order):\n");
    tlsf_block_header_t *block = pool->control.null_block;
    int block_idx = 0;

    while (block) {
        printf("  Block[%d]:\n", block_idx);
        printf("    payload_offset: %lu\n", block->payload_offset);
        printf("    block_size: %lu\n", block->block_size);
        printf("    is_free: %s\n", block->is_free ? "true" : "false");
        printf("    is_last: %s\n", block->is_last_block ? "true" : "false");
        if (!block->is_free) {
            printf("    pod_uid: %s\n", block->pod_uid);
            printf("    vnpu_id: %d\n", block->vnpu_id);
        }

        if (block->is_last_block)
            break;

        block = TLSF_GET_NEXT_PHYS_BLOCK(block, block->block_size);
        block_idx++;
    }

    printf("\nFree Blocks (FL/SL matrix):\n");
    for (int fl = 0; fl < TLSF_FL_INDEX_MAX; fl++) {
        if (!tlsf_test_bit(pool->control.fl_bitmap, fl))
            continue;

        printf("  FL[%d]:\n", fl);
        for (int sl = 0; sl < TLSF_SL_INDEX_COUNT; sl++) {
            if (!tlsf_test_bit(pool->control.sl_bitmap[fl], sl))
                continue;

            tlsf_block_header_t *free_block = pool->control.blocks[fl][sl];
            printf("    SL[%d]: ", sl);

            int count = 0;
            while (free_block) {
                printf("offset=%lu(size=%lu) ", free_block->payload_offset, free_block->block_size);
                free_block = free_block->next_free_block;
                count++;
            }
            printf("(count=%d)\n", count);
        }
    }

    printf("\n=== End Dump ===\n\n");

    pthread_mutex_unlock(&pool->lock);
}