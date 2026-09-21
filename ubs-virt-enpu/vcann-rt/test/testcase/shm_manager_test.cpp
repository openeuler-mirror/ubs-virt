/*
* Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
* ubs-virt-enpu is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A SPECIFIC PURPOSE.
* See the Mulan PSL v2 for more details.
*/

#include "shm_manager.h"
#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>
#include <atomic>
#include "log.h"

class ShmManagerTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        std::cout << "ShmManager test start" << std::endl;
    }

    static void TearDownTestCase()
    {
        std::cout << "ShmManager test end" << std::endl;
    }

    void SetUp()
    {
        (void)sprintf_s(g_log_config.log_dir, sizeof(g_log_config.log_dir), "%s", "../__build/log/enpu/");
    }

    void TearDown() {}
};

TEST_F(ShmManagerTest, ShmGetEntryNullState)
{
    vnpu_shm_entry_t *entry = shm_get_entry(NULL, 5);
    EXPECT_EQ(entry, nullptr);
}

TEST_F(ShmManagerTest, ShmGetEntryInvalidVnpuId)
{
    shm_state_t state = {0};

    vnpu_shm_entry_t *entry = shm_get_entry(&state, -1);
    EXPECT_EQ(entry, nullptr);

    entry = shm_get_entry(&state, MAX_VNPU_PER_DIE);
    EXPECT_EQ(entry, nullptr);
}

TEST_F(ShmManagerTest, ShmGetEntryNotInBitmap)
{
    shm_state_t state = {0};

    vnpu_shm_entry_t *entry = shm_get_entry(&state, 5);
    EXPECT_EQ(entry, nullptr);
}

TEST_F(ShmManagerTest, ShmGetEntryFound)
{
    shm_state_t state = {0};
    vnpu_bitmap_set(state.vnpu_bitmap, 5);
    state.entries[5].vnpu_id = 5;
    state.entries[5].hbm_request = 8192;
    state.entries[5].hbm_limit = 32768;

    vnpu_shm_entry_t *entry = shm_get_entry(&state, 5);
    EXPECT_NE(entry, nullptr);
    EXPECT_EQ(entry->vnpu_id, 5);
    EXPECT_EQ(entry->hbm_request, 8192);
    EXPECT_EQ(entry->hbm_limit, 32768);
    EXPECT_EQ(entry, &state.entries[5]);
}

TEST_F(ShmManagerTest, ShmUpdateUsedNullState)
{
    int rc = shm_update_used(NULL, 5, 1024);
    EXPECT_EQ(rc, ENPU_FAIL);
}

TEST_F(ShmManagerTest, ShmUpdateUsedEntryNotInBitmap)
{
    shm_state_t state = {0};

    int rc = shm_update_used(&state, 5, 1024);
    EXPECT_EQ(rc, ENPU_FAIL);
}

TEST_F(ShmManagerTest, ShmUpdateUsedSuccess)
{
    shm_state_t state = {0};
    vnpu_bitmap_set(state.vnpu_bitmap, 5);

    int rc = shm_update_used(&state, 5, 4096);
    EXPECT_EQ(rc, ENPU_SUCCESS);
    EXPECT_EQ(state.entries[5].hbm_used, 4096);
}

TEST_F(ShmManagerTest, ShmStateStructSize)
{
    EXPECT_GT(sizeof(shm_state_t), sizeof(vnpu_shm_entry_t) * MAX_VNPU_PER_DIE);
}

TEST_F(ShmManagerTest, ShmGetDynamicFreeNullState)
{
    uint64_t freeSize = shm_get_dynamic_free(NULL);
    EXPECT_EQ(freeSize, 0);
}

TEST_F(ShmManagerTest, ShmGetDynamicFreeEmpty)
{
    shm_state_t state = {0};
    state.hbm_total = 65536;

    uint64_t freeSize = shm_get_dynamic_free(&state);
    EXPECT_EQ(freeSize, 65536);
}

TEST_F(ShmManagerTest, ShmGetDynamicFreeSingleEntry)
{
    shm_state_t state = {0};
    state.hbm_total = 65536 + MB_TO_B; /* 容纳 1MB 配额后剩 65536 */
    vnpu_bitmap_set(state.vnpu_bitmap, 5);
    state.entries[5].hbm_request = (uint64_t)1; /* 1MB */
    state.entries[5].hbm_used = (uint64_t)4096; /* used < request, 不额外扣减 */

    uint64_t freeSize = shm_get_dynamic_free(&state);
    EXPECT_EQ(freeSize, (uint64_t)65536);
}

TEST_F(ShmManagerTest, ShmGetDynamicFreeMultipleEntries)
{
    shm_state_t state = {0};
    state.hbm_total = 65536 + 3 * MB_TO_B;
    vnpu_bitmap_set(state.vnpu_bitmap, 5);
    state.entries[5].hbm_request = (uint64_t)1; /* 1MB */
    state.entries[5].hbm_used = (uint64_t)8192;
    vnpu_bitmap_set(state.vnpu_bitmap, 10);
    state.entries[10].hbm_request = (uint64_t)2; /* 2MB */
    state.entries[10].hbm_used = (uint64_t)4096;

    uint64_t freeSize = shm_get_dynamic_free(&state);
    EXPECT_EQ(freeSize, 65536);
}

TEST_F(ShmManagerTest, ShmGetDynamicFreeOverUsedDeducted)
{
    shm_state_t state = {0};
    state.hbm_total = 65536 + MB_TO_B + 4096;
    vnpu_bitmap_set(state.vnpu_bitmap, 5);
    state.entries[5].hbm_request = (uint64_t)1; /* 1MB */
    state.entries[5].hbm_used = MB_TO_B + 4096; /* 超用 4096 字节 */

    uint64_t freeSize = shm_get_dynamic_free(&state);
    EXPECT_EQ(freeSize, 65536);
}

TEST_F(ShmManagerTest, ShmGetDynamicFreeExhausted)
{
    shm_state_t state = {0};
    state.hbm_total = 4096;
    vnpu_bitmap_set(state.vnpu_bitmap, 5);
    state.entries[5].hbm_request = (uint64_t)1; /* 1MB, 已超出 total */

    uint64_t freeSize = shm_get_dynamic_free(&state);
    EXPECT_EQ(freeSize, 0);
}

TEST_F(ShmManagerTest, ShmGetHbmRequestFreeNullState)
{
    uint64_t freeSize = shm_get_hbm_request_free(NULL);
    EXPECT_EQ(freeSize, 0);
}

TEST_F(ShmManagerTest, ShmSetAndGetSwapped)
{
    shm_state_t state = {0};
    vnpu_bitmap_set(state.vnpu_bitmap, 5);

    EXPECT_FALSE(shm_get_swapped(&state, 5));
    shm_set_swapped(&state, 5, true);
    EXPECT_TRUE(shm_get_swapped(&state, 5));
    shm_set_swapped(&state, 5, false);
    EXPECT_FALSE(shm_get_swapped(&state, 5));
}
