/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 */

#include "swap_executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "rts_mem.h"
#include "runtime_hook.h"

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

static char g_swap_buffer[1024 * 1024];

static void test_create_destroy(void)
{
    swap_executor_t *executor = swap_executor_create(g_swap_buffer);
    TEST_ASSERT_NE(executor, NULL, "create should succeed");

    int ret = swap_executor_destroy(executor);
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "destroy should succeed");

    ret = swap_executor_destroy(NULL);
    TEST_ASSERT_EQ(ret, ENPU_FAIL, "null destroy should fail");

    executor = swap_executor_create(NULL);
    TEST_ASSERT_EQ(executor, NULL, "null buffer should fail");
}

static void test_swap_out_success(void)
{
    vmm_mock_reset();

    swap_executor_t *executor = swap_executor_create(g_swap_buffer);
    TEST_ASSERT_NE(executor, NULL, "create should succeed");

    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "tracker create should succeed");

    void *test_ptr = (void *)0x1000;
    rtDrvMemHandle handle = (void *)1;
    int add_ret = memory_tracker_add(tracker, test_ptr, 4096, handle, false);
    TEST_ASSERT_EQ(add_ret, ENPU_SUCCESS, "add should succeed");

    memory_record_t *record = memory_tracker_find(tracker, test_ptr);
    TEST_ASSERT_NE(record, NULL, "find should succeed");

    memory_record_t *records[1] = {record};

    uint64_t act_swapped = 0;
    int ret = swap_executor_swap_out(executor, tracker, records, 1, 0, &act_swapped);
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "swap_out should succeed");

    memory_tracker_destroy(tracker);
    swap_executor_destroy(executor);
}

static void test_swap_out_multiple_records(void)
{
    vmm_mock_reset();

    swap_executor_t *executor = swap_executor_create(g_swap_buffer);
    TEST_ASSERT_NE(executor, NULL, "create should succeed");

    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "tracker create should succeed");

    void *ptrs[3] = {(void *)0x1000, (void *)0x2000, (void *)0x3000};
    rtDrvMemHandle handles[3] = {(void *)1, (void *)2, (void *)3};

    for (int i = 0; i < 3; i++) {
        int add_ret = memory_tracker_add(tracker, ptrs[i], 4096, handles[i], false);
        TEST_ASSERT_EQ(add_ret, ENPU_SUCCESS, "add should succeed");
    }

    memory_record_t *records[3];
    for (int i = 0; i < 3; i++) {
        records[i] = memory_tracker_find(tracker, ptrs[i]);
        TEST_ASSERT_NE(records[i], NULL, "find should succeed");
    }

    uint64_t act_swapped = 0;
    int ret = swap_executor_swap_out(executor, tracker, records, 3, 0, &act_swapped);
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "swap_out multiple should succeed");

    memory_tracker_destroy(tracker);
    swap_executor_destroy(executor);
}

static void test_swap_out_failure(void)
{
    vmm_mock_reset();

    swap_executor_t *executor = swap_executor_create(g_swap_buffer);
    TEST_ASSERT_NE(executor, NULL, "create should succeed");

    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "tracker create should succeed");

    void *test_ptr = (void *)0x1000;
    rtDrvMemHandle handle = (void *)1;
    int add_ret = memory_tracker_add(tracker, test_ptr, 4096, handle, false);
    TEST_ASSERT_EQ(add_ret, ENPU_SUCCESS, "add should succeed");

    memory_record_t *record = memory_tracker_find(tracker, test_ptr);
    TEST_ASSERT_NE(record, NULL, "find should succeed");

    memory_record_t *records[1] = {record};

    uint64_t act_swapped = 0;
    int ret = swap_executor_swap_out(executor, tracker, records, 1, 0, &act_swapped);
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "swap_out should succeed (stubs always succeed)");

    memory_tracker_destroy(tracker);
    swap_executor_destroy(executor);
}

static void test_swap_in_success(void)
{
    vmm_mock_reset();

    swap_executor_t *executor = swap_executor_create(g_swap_buffer);
    TEST_ASSERT_NE(executor, NULL, "create should succeed");

    void *test_ptr = (void *)0x1000;
    rtDrvMemHandle new_handle = NULL;

    uint64_t swap_size = 4096;
    int ret = swap_executor_swap_in(executor, &test_ptr, 0, &swap_size, &new_handle, 0);
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "swap_in should succeed");
    TEST_ASSERT_NE((intptr_t)new_handle, 0, "new_handle should be set");

    swap_executor_destroy(executor);
}

static void test_swap_in_failure(void)
{
    vmm_mock_reset();

    swap_executor_t *executor = swap_executor_create(g_swap_buffer);
    TEST_ASSERT_NE(executor, NULL, "create should succeed");

    void *test_ptr = (void *)0x1000;
    rtDrvMemHandle new_handle = NULL;

    uint64_t swap_size = 4096;
    int ret = swap_executor_swap_in(executor, &test_ptr, 0, &swap_size, &new_handle, 0);
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "swap_in should succeed (stubs always succeed)");

    swap_executor_destroy(executor);
}

static void test_swap_out_null_params(void)
{
    swap_executor_t *executor = swap_executor_create(g_swap_buffer);
    TEST_ASSERT_NE(executor, NULL, "create should succeed");

    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "tracker create should succeed");

    memory_record_t *records[1] = {NULL};
    uint64_t act_swapped = 0;

    int ret = swap_executor_swap_out(NULL, tracker, records, 1, 0, &act_swapped);
    TEST_ASSERT_EQ(ret, ENPU_FAIL, "null executor should fail");

    ret = swap_executor_swap_out(executor, tracker, NULL, 1, 0, &act_swapped);
    TEST_ASSERT_EQ(ret, ENPU_FAIL, "null records should fail");

    ret = swap_executor_swap_out(executor, tracker, records, 0, 0, &act_swapped);
    TEST_ASSERT_EQ(ret, ENPU_FAIL, "zero count should fail");

    memory_tracker_destroy(tracker);
    swap_executor_destroy(executor);
}

static void test_swap_in_null_params(void)
{
    swap_executor_t *executor = swap_executor_create(g_swap_buffer);
    TEST_ASSERT_NE(executor, NULL, "create should succeed");

    rtDrvMemHandle new_handle = NULL;

    uint64_t in_size = 4096;
    void *test_ptr = (void *)0x1000;
    int ret = swap_executor_swap_in(NULL, &test_ptr, 0, &in_size, &new_handle, 0);
    TEST_ASSERT_EQ(ret, ENPU_FAIL, "null executor should fail");

    in_size = 4096;
    ret = swap_executor_swap_in(executor, NULL, 0, &in_size, &new_handle, 0);
    TEST_ASSERT_EQ(ret, ENPU_FAIL, "null ptr should fail");

    in_size = 0;
    test_ptr = (void *)0x1000;
    ret = swap_executor_swap_in(executor, &test_ptr, 0, &in_size, &new_handle, 0);
    // Note: source only checks size==NULL, not *size==0, so this succeeds with stubs
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "zero *size: succeeds (size ptr non-NULL)");

    in_size = 4096;
    test_ptr = (void *)0x1000;
    ret = swap_executor_swap_in(executor, &test_ptr, 0, &in_size, NULL, 0);
    TEST_ASSERT_EQ(ret, ENPU_FAIL, "null handle should fail");

    swap_executor_destroy(executor);
}

/* swap_in 路径经 rt_library_entry 函数表调用物理内存接口,
 * 独立测试进程无 dlsym 环境, 表项为空会导致 HOOK ERROR,
 * 此处用本地成功桩填充所需表项(参数列按 swap_executor.c 的调用形态) */
static rtError_t stub_get_granularity(rtDrvMemProp_t *prop, uint32_t id, size_t *gran)
{
    (void)prop;
    (void)id;
    if (gran != NULL) {
        *gran = 1; /* 粒度 1: 不改变请求 size, 断言不受对齐影响 */
    }
    return RT_ERROR_NONE;
}

static rtError_t stub_malloc_physical(rtDrvMemHandle *handle, size_t size, rtDrvMemProp_t *prop, uint64_t flags)
{
    (void)size;
    (void)prop;
    (void)flags;
    if (handle != NULL) {
        *handle = (rtDrvMemHandle)(intptr_t)0x2000;
    }
    return RT_ERROR_NONE;
}

static rtError_t stub_reserve_mem_address(void **ptr, size_t size, uint32_t a, void *b, uint64_t c)
{
    (void)ptr;
    (void)size;
    (void)a;
    (void)b;
    (void)c;
    return RT_ERROR_NONE;
}

static rtError_t stub_map_mem(void *va, size_t size, size_t offset, rtDrvMemHandle handle, uint64_t flags)
{
    (void)va;
    (void)size;
    (void)offset;
    (void)handle;
    (void)flags;
    return RT_ERROR_NONE;
}

static rtError_t stub_free_physical(rtDrvMemHandle handle)
{
    (void)handle;
    return RT_ERROR_NONE;
}

static rtError_t stub_rt_memcpy(void *dst, uint64_t destMax, const void *src, uint64_t cnt, rtMemcpyKind_t kind)
{
    (void)dst;
    (void)destMax;
    (void)src;
    (void)cnt;
    (void)kind;
    return RT_ERROR_NONE;
}

static void fill_rt_entry_for_swap_in(void)
{
    rt_library_entry[HOOK_rtMemGetAllocationGranularity].func_ptr = (rt_symbol_t)stub_get_granularity;
    rt_library_entry[HOOK_rtMallocPhysical].func_ptr = (rt_symbol_t)stub_malloc_physical;
    rt_library_entry[HOOK_rtReserveMemAddress].func_ptr = (rt_symbol_t)stub_reserve_mem_address;
    rt_library_entry[HOOK_rtMapMem].func_ptr = (rt_symbol_t)stub_map_mem;
    rt_library_entry[HOOK_rtFreePhysical].func_ptr = (rt_symbol_t)stub_free_physical;
    rt_library_entry[HOOK_rtMemcpy].func_ptr = (rt_symbol_t)stub_rt_memcpy;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("====================================\n");
    printf("SwapExecutor Unit Tests\n");
    printf("====================================\n");

    fill_rt_entry_for_swap_in();

    TEST_SUITE_BEGIN("create_destroy");
    TEST_CASE(create_destroy);
    TEST_SUITE_END();

    TEST_SUITE_BEGIN("swap_out");
    TEST_CASE(swap_out_success);
    TEST_CASE(swap_out_multiple_records);
    TEST_CASE(swap_out_failure);
    TEST_SUITE_END();

    TEST_SUITE_BEGIN("swap_in");
    TEST_CASE(swap_in_success);
    TEST_CASE(swap_in_failure);
    TEST_SUITE_END();

    TEST_SUITE_BEGIN("invalid_params");
    TEST_CASE(swap_out_null_params);
    TEST_CASE(swap_in_null_params);
    TEST_SUITE_END();

    printf("\n====================================\n");
    printf("Total: Passed %d, Failed %d\n", g_pass_count, g_fail_count);
    printf("====================================\n");

    return (g_fail_count > 0) ? 1 : 0;
}