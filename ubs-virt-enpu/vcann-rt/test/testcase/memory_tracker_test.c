/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 */

#include "memory_tracker.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

static int g_pass_count = 0;
static int g_fail_count = 0;

#define TEST_ASSERT(cond, msg)                                                \
    do {                                                                      \
        if (!(cond)) {                                                        \
            fprintf(stderr, "  FAIL: %s:%d - %s\n", __FILE__, __LINE__, msg); \
            g_fail_count++;                                                   \
            return;                                                           \
        }                                                                     \
        g_pass_count++;                                                       \
    } while (0)

#define TEST_ASSERT_EQ(a, b, msg) TEST_ASSERT((a) == (b), msg)
#define TEST_ASSERT_NE(a, b, msg) TEST_ASSERT((a) != (b), msg)

#define TEST_SUITE_BEGIN(name) printf("\n[%s]\n", name)
#define TEST_SUITE_END() printf("  Done\n")
#define TEST_CASE(name)        \
    printf("  - %s: ", #name); \
    test_##name();             \
    printf("OK\n")

static void test_create_destroy(void)
{
    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "create should succeed");

    int ret = memory_tracker_destroy(tracker);
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "destroy should succeed");

    ret = memory_tracker_destroy(NULL);
    TEST_ASSERT_EQ(ret, ENPU_FAIL, "null destroy should fail");
}

static void test_add_find_record(void)
{
    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "create should succeed");

    void *test_ptr = (void *)0x1000;
    uint64_t test_size = 4096;
    rtDrvMemHandle test_handle = (void *)1;

    int ret = memory_tracker_add(tracker, test_ptr, test_size, test_handle, false);
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "add should succeed");

    memory_record_t *record = memory_tracker_find(tracker, test_ptr);
    TEST_ASSERT_NE(record, NULL, "find should succeed");
    TEST_ASSERT_EQ(record->ptr, test_ptr, "ptr should match");
    TEST_ASSERT_EQ(record->size, test_size, "size should match");
    TEST_ASSERT_EQ((intptr_t)record->handle, 1, "handle should match");
    TEST_ASSERT_EQ(record->is_swapped, false, "is_swapped should be false");
    TEST_ASSERT_EQ(record->swap_offset, 0, "swap_offset should be 0");

    memory_tracker_destroy(tracker);
}

static void test_add_multiple_records(void)
{
    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "create should succeed");

    void *ptrs[3] = {(void *)0x1000, (void *)0x2000, (void *)0x3000};
    rtDrvMemHandle handles[3] = {(void *)1, (void *)2, (void *)3};

    for (int i = 0; i < 3; i++) {
        int ret = memory_tracker_add(tracker, ptrs[i], 4096 * (i + 1), handles[i], false);
        TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "add should succeed");
    }

    for (int i = 0; i < 3; i++) {
        memory_record_t *record = memory_tracker_find(tracker, ptrs[i]);
        TEST_ASSERT_NE(record, NULL, "find should succeed");
        TEST_ASSERT_EQ((intptr_t)record->handle, i + 1, "handle should match");
    }

    memory_tracker_destroy(tracker);
}

static void test_find_nonexistent(void)
{
    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "create should succeed");

    memory_record_t *record = memory_tracker_find(tracker, (void *)0x1000);
    TEST_ASSERT_EQ(record, NULL, "find nonexistent should return null");

    memory_tracker_destroy(tracker);
}

static void test_add_null_params(void)
{
    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "create should succeed");

    rtDrvMemHandle handle = (void *)1;

    int ret = memory_tracker_add(NULL, (void *)0x1000, 4096, handle, false);
    TEST_ASSERT_EQ(ret, ENPU_FAIL, "null tracker should fail");

    ret = memory_tracker_add(tracker, NULL, 4096, handle, false);
    TEST_ASSERT_EQ(ret, ENPU_FAIL, "null ptr should fail");

    ret = memory_tracker_add(tracker, (void *)0x1000, 0, handle, false);
    TEST_ASSERT_EQ(ret, ENPU_FAIL, "zero size should fail");

    memory_tracker_destroy(tracker);
}

static void test_remove_record(void)
{
    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "create should succeed");

    void *test_ptr = (void *)0x1000;
    rtDrvMemHandle handle = (void *)1;

    memory_tracker_add(tracker, test_ptr, 4096, handle, false);

    memory_record_t *record = memory_tracker_find(tracker, test_ptr);
    TEST_ASSERT_NE(record, NULL, "find should succeed before remove");

    int ret = memory_tracker_remove(tracker, test_ptr);
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "remove should succeed");

    record = memory_tracker_find(tracker, test_ptr);
    TEST_ASSERT_EQ(record, NULL, "find should return null after remove");

    memory_tracker_destroy(tracker);
}

static void test_remove_nonexistent(void)
{
    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "create should succeed");

    int ret = memory_tracker_remove(tracker, (void *)0x1000);
    // Source returns ENPU_SUCCESS when record not found (idempotent remove)
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "remove nonexistent is idempotent (returns success)");

    ret = memory_tracker_remove(NULL, (void *)0x1000);
    TEST_ASSERT_EQ(ret, ENPU_FAIL, "remove with null tracker should fail");

    memory_tracker_destroy(tracker);
}

static void test_mark_swapped(void)
{
    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "create should succeed");

    void *test_ptr = (void *)0x1000;
    rtDrvMemHandle handle = (void *)1;

    memory_tracker_add(tracker, test_ptr, 4096, handle, false);

    memory_record_t *record = memory_tracker_find(tracker, test_ptr);
    TEST_ASSERT_EQ(record->is_swapped, false, "is_swapped should be false initially");

    int ret = memory_tracker_mark_swapped(tracker, test_ptr, 102400);
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "mark_swapped should succeed");

    record = memory_tracker_find(tracker, test_ptr);
    TEST_ASSERT_EQ(record->is_swapped, true, "is_swapped should be true");
    TEST_ASSERT_EQ(record->swap_offset, 102400, "swap_offset should match");

    memory_tracker_destroy(tracker);
}

static void test_mark_swapped_nonexistent(void)
{
    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "create should succeed");

    int ret = memory_tracker_mark_swapped(tracker, (void *)0x1000, 102400);
    TEST_ASSERT_EQ(ret, ENPU_FAIL, "mark_swapped nonexistent should fail");

    memory_tracker_destroy(tracker);
}

static void test_mark_active(void)
{
    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "create should succeed");

    void *test_ptr = (void *)0x1000;
    rtDrvMemHandle handle1 = (void *)1;

    memory_tracker_add(tracker, test_ptr, 4096, handle1, false);
    memory_tracker_mark_swapped(tracker, test_ptr, 102400);

    memory_record_t *record = memory_tracker_find(tracker, test_ptr);
    TEST_ASSERT_EQ(record->is_swapped, true, "is_swapped should be true before mark_active");

    rtDrvMemHandle handle2 = (void *)2;
    int ret = memory_tracker_mark_active(tracker, test_ptr, handle2);
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "mark_active should succeed");

    record = memory_tracker_find(tracker, test_ptr);
    TEST_ASSERT_EQ(record->is_swapped, false, "is_swapped should be false after mark_active");
    TEST_ASSERT_EQ((intptr_t)record->handle, 2, "handle should be updated");
    TEST_ASSERT_EQ(record->swap_offset, 0, "swap_offset should be cleared");

    memory_tracker_destroy(tracker);
}

static void test_mark_active_nonexistent(void)
{
    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "create should succeed");

    rtDrvMemHandle handle = (void *)1;
    int ret = memory_tracker_mark_active(tracker, (void *)0x1000, handle);
    TEST_ASSERT_EQ(ret, ENPU_FAIL, "mark_active nonexistent should fail");

    memory_tracker_destroy(tracker);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("====================================\n");
    printf("MemoryTracker Unit Tests\n");
    printf("====================================\n");

    TEST_SUITE_BEGIN("create_destroy");
    TEST_CASE(create_destroy);
    TEST_SUITE_END();

    TEST_SUITE_BEGIN("add_find");
    TEST_CASE(add_find_record);
    TEST_CASE(add_multiple_records);
    TEST_CASE(find_nonexistent);
    TEST_SUITE_END();

    TEST_SUITE_BEGIN("invalid_params");
    TEST_CASE(add_null_params);
    TEST_SUITE_END();

    TEST_SUITE_BEGIN("remove");
    TEST_CASE(remove_record);
    TEST_CASE(remove_nonexistent);
    TEST_SUITE_END();

    TEST_SUITE_BEGIN("mark_swapped");
    TEST_CASE(mark_swapped);
    TEST_CASE(mark_swapped_nonexistent);
    TEST_SUITE_END();

    TEST_SUITE_BEGIN("mark_active");
    TEST_CASE(mark_active);
    TEST_CASE(mark_active_nonexistent);
    TEST_SUITE_END();

    printf("\n====================================\n");
    printf("Total: Passed %d, Failed %d\n", g_pass_count, g_fail_count);
    printf("====================================\n");

    return (g_fail_count > 0) ? 1 : 0;
}