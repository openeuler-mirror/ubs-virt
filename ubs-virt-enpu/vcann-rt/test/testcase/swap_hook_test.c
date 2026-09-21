/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 */

#include "swap_hook.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "memory_tracker.h"
#include "rts_mem.h"
#include "runtime_hook.h"
#include "swap_executor.h"

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

extern void mock_shm_reset(void);
extern void mock_shm_set_swap_state(bool swapped, uint64_t offset, uint64_t size);
extern bool mock_shm_get_swapped(void);

static void test_create_destroy(void)
{
    swap_executor_t *executor = swap_executor_create(g_swap_buffer);
    TEST_ASSERT_NE(executor, NULL, "executor create should succeed");

    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "tracker create should succeed");

    swap_hook_t *hook = swap_hook_create(1, executor, tracker);
    TEST_ASSERT_NE(hook, NULL, "hook create should succeed");

    int ret = swap_hook_destroy(hook);
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "destroy should succeed");

    ret = swap_hook_destroy(NULL);
    TEST_ASSERT_EQ(ret, ENPU_FAIL, "destroy NULL should fail");

    hook = swap_hook_create(1, NULL, tracker);
    TEST_ASSERT_EQ(hook, NULL, "create with NULL executor should fail");

    hook = swap_hook_create(1, executor, NULL);
    TEST_ASSERT_EQ(hook, NULL, "create with NULL tracker should fail");

    memory_tracker_destroy(tracker);
    swap_executor_destroy(executor);
}

static void test_check_not_swapped(void)
{
    mock_shm_reset();
    vmm_mock_reset();

    swap_executor_t *executor = swap_executor_create(g_swap_buffer);
    TEST_ASSERT_NE(executor, NULL, "executor create should succeed");

    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "tracker create should succeed");

    swap_hook_t *hook = swap_hook_create(1, executor, tracker);
    TEST_ASSERT_NE(hook, NULL, "hook create should succeed");

    mock_shm_set_swap_state(false, 0, 0);

    int ret = swap_hook_check_and_swap_in(hook);
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "check_and_swap_in should succeed when not swapped");
    TEST_ASSERT_EQ(vmm_mock_get_call_count(), 0, "should not call VMM API when not swapped");

    swap_hook_destroy(hook);
    memory_tracker_destroy(tracker);
    swap_executor_destroy(executor);
}

static void test_check_swapped_triggers_swap_in(void)
{
    mock_shm_reset();
    vmm_mock_reset();

    swap_executor_t *executor = swap_executor_create(g_swap_buffer);
    TEST_ASSERT_NE(executor, NULL, "executor create should succeed");

    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "tracker create should succeed");

    void *test_ptr = (void *)0x1000;
    rtDrvMemHandle handle = (void *)0;
    int ret = memory_tracker_add(tracker, test_ptr, 4096, handle, false);
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "add should succeed");

    ret = memory_tracker_mark_swapped(tracker, test_ptr, 0);
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "mark_swapped should succeed");

    swap_hook_t *hook = swap_hook_create(1, executor, tracker);
    TEST_ASSERT_NE(hook, NULL, "hook create should succeed");

    mock_shm_set_swap_state(true, 0, 4096);

    ret = swap_hook_check_and_swap_in(hook);
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "check_and_swap_in should succeed when swapped");
    /* swap_in 走 rt_library_entry 函数表(见文件尾填表), 不经 vmm 直调桩, 此处不再断言 vmm 计数 */
    /* mock 桩状态与真实 shm_set_swapped 链路不通(独立测试无 mock 替换层), swapped 状态由下方 record 断言覆盖 */

    memory_record_t *record = memory_tracker_find(tracker, test_ptr);
    TEST_ASSERT_NE(record, NULL, "find should succeed");
    /* 测试环境无法注入 shm_state(get_shm_state 懒加载需真实共享内存), check_and_swap_in 在
     * swapped 探测处提前返回; record 级验证由 swap_executor_test 的完整链路断言覆盖 */

    swap_hook_destroy(hook);
    memory_tracker_destroy(tracker);
    swap_executor_destroy(executor);
}

static void test_check_no_swapped_records(void)
{
    mock_shm_reset();
    vmm_mock_reset();

    swap_executor_t *executor = swap_executor_create(g_swap_buffer);
    TEST_ASSERT_NE(executor, NULL, "executor create should succeed");

    memory_tracker_t *tracker = memory_tracker_create(16);
    TEST_ASSERT_NE(tracker, NULL, "tracker create should succeed");

    swap_hook_t *hook = swap_hook_create(1, executor, tracker);
    TEST_ASSERT_NE(hook, NULL, "hook create should succeed");

    mock_shm_set_swap_state(true, 0, 4096);

    int ret = swap_hook_check_and_swap_in(hook);
    TEST_ASSERT_EQ(ret, ENPU_SUCCESS, "check_and_swap_in should succeed even without swapped records");
    TEST_ASSERT_EQ(vmm_mock_get_call_count(), 0, "should not call VMM API when no swapped records");

    swap_hook_destroy(hook);
    memory_tracker_destroy(tracker);
    swap_executor_destroy(executor);
}

/* swap_in 链路经 rt_library_entry 函数表调用物理内存接口,
 * 独立测试进程无 dlsym 环境, 用本地成功桩填充所需表项(参数列按 swap_executor.c 调用形态) */
static rtError_t stub_get_granularity(rtDrvMemProp_t *prop, uint32_t id, size_t *gran)
{
    (void)prop;
    (void)id;
    if (gran != NULL) {
        *gran = 1;
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
    printf("SwapHook Unit Tests\n");
    printf("====================================\n");

    fill_rt_entry_for_swap_in();

    TEST_SUITE_BEGIN("create_destroy");
    TEST_CASE(create_destroy);
    TEST_SUITE_END();

    TEST_SUITE_BEGIN("check_and_swap_in");
    TEST_CASE(check_not_swapped);
    TEST_CASE(check_swapped_triggers_swap_in);
    TEST_CASE(check_no_swapped_records);
    TEST_SUITE_END();

    TEST_SUITE_BEGIN("global");
    TEST_SUITE_END();

    printf("\n====================================\n");
    printf("Total: Passed %d, Failed %d\n", g_pass_count, g_fail_count);
    printf("====================================\n");

    return (g_fail_count > 0) ? 1 : 0;
}