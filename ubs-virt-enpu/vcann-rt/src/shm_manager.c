/*
* Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
* ubs-virt-enpu is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v2 for more details.
*/

#include "shm_manager.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "log.h"
#include "securec.h"

#define MAX_RETRY_COUNT 50
#define RETRY_INTERVAL_MS 100000

shm_state_t *g_shm_state;

shm_state_t *shm_state_posix_shm_attach(int phy_id, const char *die_id)
{
    if (die_id == NULL || strlen(die_id) == 0) {
        LOG_ERROR("die_id is NULL or empty");
        return NULL;
    }

    if (phy_id < 0) {
        LOG_ERROR("Invalid phy_id: %d", phy_id);
        return NULL;
    }

    char shm_name[128];
    int ret = snprintf_s(shm_name, sizeof(shm_name), sizeof(shm_name) - 1, "/shm_state-%d-%s", phy_id, die_id);
    if (ret < 0) {
        LOG_ERROR("Failed to construct POSIX shm name for phy_id=%d, die_id=%s", phy_id, die_id);
        return NULL;
    }

    shm_state_t *state = NULL;
    int fd = -1;

    for (int retry = 0; retry < MAX_RETRY_COUNT; retry++) {
        fd = shm_open(shm_name, O_RDWR, 0644);
        if (fd < 0) {
            if (retry < MAX_RETRY_COUNT - 1) {
                LOG_DEBUG("POSIX shm %s not found, retry %d/%d", shm_name, retry + 1, MAX_RETRY_COUNT);
                usleep(RETRY_INTERVAL_MS);
                continue;
            }
            LOG_DEBUG("POSIX shm %s not available after %d retries", shm_name, MAX_RETRY_COUNT);
            return NULL;
        }

        void *addr = mmap(NULL, sizeof(shm_state_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) {
            LOG_ERROR("Failed to mmap POSIX shm %s, error: %s", shm_name, strerror(errno));
            close(fd);
            return NULL;
        }

        state = (shm_state_t *)addr;

        if (!atomic_load(&state->initialized)) {
            munmap(state, sizeof(shm_state_t));
            close(fd);
            state = NULL;
            fd = -1;

            if (retry < MAX_RETRY_COUNT - 1) {
                LOG_DEBUG("shm_state not initialized, retry %d/%d", retry + 1, MAX_RETRY_COUNT);
                usleep(RETRY_INTERVAL_MS);
                continue;
            }
            LOG_ERROR("shm_state not initialized after %d retries", MAX_RETRY_COUNT);
            return NULL;
        }

        if (state->version != SHM_STATE_VERSION) {
            LOG_ERROR("shm_state version mismatch: expected %d, got %d", SHM_STATE_VERSION, state->version);
            munmap(state, sizeof(shm_state_t));
            close(fd);
            return NULL;
        }

        break;
    }

    close(fd);

    LOG_INFO("Successfully attached shm_state: phy_id=%d, shm_id=%s, version=%d, hbm_total=%lu", state->phy_id,
             state->shm_id, state->version, state->hbm_total);

    return state;
}

void shm_state_posix_shm_detach(shm_state_t *state)
{
    if (state == NULL) {
        return;
    }

    munmap(state, sizeof(shm_state_t));
    LOG_INFO("Detached shm_state mapping");
}

vnpu_shm_entry_t *shm_get_entry(shm_state_t *state, int32_t vnpu_id)
{
    if (state == NULL) {
        LOG_ERROR("shm_state is NULL");
        return NULL;
    }

    if (vnpu_id < 0 || vnpu_id >= MAX_VNPU_PER_DIE) {
        LOG_ERROR("Invalid vnpu_id: %d", vnpu_id);
        return NULL;
    }

    if (!vnpu_bitmap_test(state->vnpu_bitmap, vnpu_id)) {
        LOG_WARN("vnpu_id %d not allocated in shm_state", vnpu_id);
        return NULL;
    }

    return &state->entries[vnpu_id];
}

int shm_update_hbm_request(shm_state_t *state, int32_t vnpu_id, uint64_t request_quota)
{
    if (state == NULL) {
        LOG_ERROR("shm_state is NULL");
        return ENPU_FAIL;
    }

    vnpu_shm_entry_t *entry = shm_get_entry(state, vnpu_id);
    if (entry == NULL) {
        LOG_ERROR("Failed to get entry for vnpu_id: %d", vnpu_id);
        return ENPU_FAIL;
    }

    atomic_store(&entry->hbm_request, request_quota);
    LOG_DEBUG("Updated hbm_request for vnpu_id %d to %lu", vnpu_id, request_quota);

    return ENPU_SUCCESS;
}

int shm_update_used(shm_state_t *state, int32_t vnpu_id, uint64_t used)
{
    if (state == NULL) {
        LOG_ERROR("shm_state is NULL");
        return ENPU_FAIL;
    }

    vnpu_shm_entry_t *entry = shm_get_entry(state, vnpu_id);
    if (entry == NULL) {
        LOG_ERROR("Failed to get entry for vnpu_id: %d", vnpu_id);
        return ENPU_FAIL;
    }

    atomic_store(&entry->hbm_used, used);
    LOG_DEBUG("Updated hbm_used for vnpu_id %d to %lu", vnpu_id, used);

    return ENPU_SUCCESS;
}

uint64_t shm_get_dynamic_free(shm_state_t *state)
{
    if (state == NULL) {
        LOG_ERROR("shm_state is NULL");
        return 0;
    }

    uint64_t total_used = 0;
    uint64_t total_hbm_request = 0;

    for (int vnpu_id = 0; vnpu_id < MAX_VNPU_PER_DIE; vnpu_id++) {
        if (vnpu_bitmap_test(state->vnpu_bitmap, vnpu_id)) {
            uint64_t used = atomic_load(&state->entries[vnpu_id].hbm_used);
            uint64_t hbm_request = atomic_load(&state->entries[vnpu_id].hbm_request) * MB_TO_B;
            if (used > hbm_request) {
                total_used = total_used + used - hbm_request;
            }
            total_hbm_request += hbm_request;
        }
    }

    /* 无符号下溢保护: 配额总和超出 hbm_total 时按 0 处理, 不回绕成巨大值 */
    if (state->hbm_total <= total_hbm_request) {
        LOG_DEBUG("Dynamic free exhausted: hbm_total=%lu, total_request=%lu", state->hbm_total, total_hbm_request);
        return 0;
    }
    uint64_t dynamic_free =
        (state->hbm_total - total_hbm_request > total_used) ? state->hbm_total - total_hbm_request - total_used : 0;
    LOG_DEBUG("Dynamic free: hbm_total=%lu, total_request=%lu, total_used=%lu, dynamic_free=%lu", state->hbm_total,
              total_hbm_request, total_used, dynamic_free);

    return dynamic_free;
}

uint64_t shm_get_hbm_request_free(shm_state_t *state)
{
    if (state == NULL) {
        LOG_ERROR("shm_state is NULL");
        return 0;
    }

    uint64_t total_hbm_request = 0;

    for (int vnpu_id = 0; vnpu_id < MAX_VNPU_PER_DIE; vnpu_id++) {
        if (vnpu_bitmap_test(state->vnpu_bitmap, vnpu_id)) {
            uint64_t hbm_request = atomic_load(&state->entries[vnpu_id].hbm_request) * MB_TO_B;
            total_hbm_request += hbm_request;
        }
    }

    /* 无符号下溢保护: 配额总和超出 hbm_total 时按 0 处理, 不回绕成巨大值 */
    uint64_t hbm_request_free = (state->hbm_total > total_hbm_request) ? state->hbm_total - total_hbm_request : 0;
    LOG_DEBUG("HBM request free: hbm_total=%lu, total_hbm_request=%lu, hbm_request_free=%lu", state->hbm_total,
              total_hbm_request, hbm_request_free);

    return hbm_request_free;
}

uint64_t shm_get_swap_size(shm_state_t *state, int32_t vnpu_id)
{
    if (state == NULL) {
        LOG_ERROR("shm_state is NULL");
        return 0;
    }

    vnpu_shm_entry_t *entry = shm_get_entry(state, vnpu_id);
    if (entry == NULL) {
        LOG_ERROR("Failed to get entry for vnpu_id: %d", vnpu_id);
        return 0;
    }

    return entry->swap_size;
}

int shm_check_swap_cmd(shm_state_t *state, int32_t *vnpu_id, int32_t *action)
{
    if (state == NULL || vnpu_id == NULL || action == NULL) {
        LOG_ERROR("Invalid parameters");
        return -1;
    }

    *vnpu_id = state->swap_cmd.target_vnpu_id;
    *action = atomic_load(&state->swap_cmd.action);

    return 0;
}

void shm_ack_swap_cmd(shm_state_t *state)
{
    if (state == NULL) {
        LOG_ERROR("shm_state is NULL");
        return;
    }

    atomic_store(&state->swap_cmd.completed, true);
    atomic_store(&state->swap_cmd.action, SWAP_ACTION_NONE);
    if (state->swap_cmd.target_vnpu_id == state->swap_out_cmd.target_vnpu_id) {
        atomic_store(&state->swap_out_cmd.completed, true);
    }
}

void shm_set_swapped(shm_state_t *state, int32_t vnpu_id, bool swapped)
{
    if (state == NULL) {
        LOG_ERROR("shm_state is NULL");
        return;
    }

    vnpu_shm_entry_t *entry = shm_get_entry(state, vnpu_id);
    if (entry == NULL) {
        LOG_ERROR("Failed to get entry for vnpu_id: %d", vnpu_id);
        return;
    }

    entry->swapped = swapped;
    LOG_INFO("Set swapped=%d for vnpu_id=%d", swapped, vnpu_id);
}

bool shm_get_swapped(shm_state_t *state, int32_t vnpu_id)
{
    if (state == NULL) {
        LOG_ERROR("shm_state is NULL");
        return false;
    }

    vnpu_shm_entry_t *entry = shm_get_entry(state, vnpu_id);
    if (entry == NULL) {
        LOG_ERROR("Failed to get entry for vnpu_id: %d", vnpu_id);
        return false;
    }

    LOG_DEBUG("Get swapped=%d for vnpu_id=%d", entry->swapped, vnpu_id);
    return entry->swapped;
}