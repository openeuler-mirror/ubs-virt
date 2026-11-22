/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 */

#include "swap_hook.h"
#include <stdlib.h>
#include <string.h>
#include "core_limiter.h"
#include "log.h"
#include "npu_manager.h"
#include "swap_buffer_shm.h"
#include "swap_monitor_thread.h"

extern void mock_shm_set_swap_state(bool swapped, uint64_t offset, uint64_t size);

static swap_hook_t *g_global_swap_hook = NULL;
static swap_buffer_mapping_t g_swap_buffer = {0};
static atomic_uint_fast32_t g_pool_generation = 0;

static int recover_swap_in_mutex(pthread_mutex_t *mutex, int rc)
{
    if (rc == EOWNERDEAD) {
        rc = pthread_mutex_consistent(mutex);
        if (rc != 0) {
            LOG_ERROR("swap_hook: pthread_mutex_consistent failed for swap_in_mutex, ret=%d", rc);
            pthread_mutex_unlock(mutex);
        }
    }
    return rc;
}

static bool swap_pool_name_is_valid(const shm_state_t *state)
{
    if (memchr(state->swap_pool_name, '\0', sizeof(state->swap_pool_name)) == NULL) {
        LOG_ERROR("swap_hook: swap pool name is not NUL-terminated, phy_id=%d", state->phy_id);
        return false;
    }
    return true;
}

swap_hook_t *swap_hook_create(int vnpu_id, void *executor, void *tracker)
{
    if (executor == NULL || tracker == NULL) {
        LOG_ERROR("swap_hook_create failed: executor=%p, tracker=%p", executor, tracker);
        return NULL;
    }

    if (vnpu_id < 0) {
        LOG_ERROR("swap_hook_create failed: invalid vnpu_id=%d", vnpu_id);
        return NULL;
    }

    swap_hook_t *hook = (swap_hook_t *)calloc(1, sizeof(swap_hook_t));
    if (hook == NULL) {
        LOG_ERROR("swap_hook_create failed: calloc returned NULL");
        return NULL;
    }

    hook->vnpu_id = vnpu_id;
    hook->executor = (swap_executor_t *)executor;
    hook->tracker = (memory_tracker_t *)tracker;

    return hook;
}

int swap_hook_destroy(swap_hook_t *hook)
{
    if (hook == NULL) {
        return ENPU_FAIL;
    }

    free(hook);
    return ENPU_SUCCESS;
}

int swap_hook_malloc_mem(swap_hook_t *hook, void **ptr, uint64_t size)
{
    if (hook == NULL) {
        LOG_ERROR("Hook is NULL.");
        return ENPU_FAIL;
    }

    rtDrvMemHandle handle;
    int ret = swap_executor_swap_in(hook->executor, ptr, 0, &size, &handle, NOT_MEMCPY_FLAG);
    CHECK_RETURN_ERROR_CODE(ret, "Malloc and map physical mem failed when malloc mem.");

    ret = memory_tracker_add(hook->tracker, *ptr, size, handle, false);
    CHECK_RETURN_ERROR_CODE(ret, "Memory tracker add record failed when malloc mem.");

    shm_state_t *state = get_shm_state();
    vnpu_shm_entry_t *entry = state != NULL ? shm_get_entry(state, get_vnpu_id()) : NULL;
    int my_slot = get_registry_slot();
    if (entry != NULL) {
        if (my_slot >= 0) {
            atomic_fetch_add(&entry->processes[my_slot].hbm_used, size);
        }
        atomic_fetch_add(&entry->hbm_used, size);
    }

    return ENPU_SUCCESS;
}

int swap_hook_malloc_physical_mem(swap_hook_t *hook, rtDrvMemHandle handle, uint64_t size, rtDrvMemProp_t *prop,
                                  uint64_t flags)
{
    if (hook == NULL) {
        LOG_ERROR("Hook is NULL.");
        return ENPU_FAIL;
    }

    int ret = memory_tracker_add(hook->tracker, handle, size, handle, true);
    CHECK_RETURN_ERROR_CODE(ret, "Memory tracker add record failed when malloc physical mem.");
    rtDrvMemProp_t rtProp = {};
    rtProp.side = prop->side;
    rtProp.devid = prop->devid;
    rtProp.module_id = prop->module_id;
    rtProp.pg_type = prop->pg_type;
    rtProp.mem_type = prop->mem_type;
    rtProp.reserve = prop->reserve;
    ret = memory_tracker_add_handle_record(hook->tracker, handle, size, rtProp, flags);
    CHECK_RETURN_ERROR_CODE(ret, "Memory tracker add handle record failed when malloc physical mem.");

    shm_state_t *state = get_shm_state();
    vnpu_shm_entry_t *entry = state != NULL ? shm_get_entry(state, get_vnpu_id()) : NULL;
    int my_slot = get_registry_slot();
    if (entry != NULL) {
        if (my_slot >= 0) {
            atomic_fetch_add(&entry->processes[my_slot].hbm_used, size);
        }
        atomic_fetch_add(&entry->hbm_used, size);
    }

    return ENPU_SUCCESS;
}

int swap_hook_check_and_swap_in(swap_hook_t *hook)
{
    if (hook == NULL) {
        /* swap 未初始化(初始化降级场景): 视为无换入需求, 调用方正常透传, 不阻断业务接口 */
        return ENPU_SUCCESS;
    }

    if (!get_swap_enabled()) {
        return ENPU_SUCCESS;
    }
    pthread_mutex_lock(&hook->tracker->lock);

    int my_slot = get_registry_slot();
    if (my_slot < 0) {
        pthread_mutex_unlock(&hook->tracker->lock);
        return ENPU_SUCCESS;
    }

    if (!shm_get_process_swapped(get_shm_state(), get_vnpu_id(), my_slot)) {
        /* 本进程未处于换出状态, 无需(重新)换入 */
        pthread_mutex_unlock(&hook->tracker->lock);
        return ENPU_SUCCESS;
    }

    LOG_INFO("swap_hook: triggering swap_in for vnpu %d, slot %d.", get_vnpu_id(), my_slot);

    shm_state_t *state = get_shm_state();
    int rc = pthread_mutex_lock(&state->swap_in_mutex);
    rc = recover_swap_in_mutex(&state->swap_in_mutex, rc);
    if (rc != 0) {
        LOG_ERROR("Failed to obtain swap in mutex lock, error code=%d.", rc);
        pthread_mutex_unlock(&hook->tracker->lock);
        return ENPU_FAIL;
    }
    uint_fast32_t em_generation = atomic_load(&state->swap_pool_generation);
    uint_fast32_t rt_generation = atomic_load(&g_pool_generation);
    bool mapped = g_swap_buffer.base != NULL;
    if (hook == g_global_swap_hook && (!mapped || em_generation != rt_generation)) {
        LOG_WARN("swap_hook: swap-in pool not ready, vnpu_id=%d slot=%d em_gen=%lu rt_gen=%lu mapped=%d", get_vnpu_id(),
                 my_slot, (unsigned long)em_generation, (unsigned long)rt_generation, mapped);
        pthread_mutex_unlock(&state->swap_in_mutex);
        pthread_mutex_unlock(&hook->tracker->lock);
        return ENPU_FAIL;
    }
    update_last_kernel_time_now();

    /* 使用本进程的换出量作为换入前的腾挪量估算 */
    process_entry_t *my_entry = &state->entries[get_vnpu_id()].processes[my_slot];
    uint64_t swap_size = atomic_load(&my_entry->swap_size);
    int ret = check_and_swap_out(swap_size, SWAP_OUT_FROM_ALL);
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("Check and swap out failed when swap in.");
        pthread_mutex_unlock(&state->swap_in_mutex);
        pthread_mutex_unlock(&hook->tracker->lock);
        return ret;
    }

    /* 恢复量在循环内按实际换入成功的记录大小累加(pending_restore_bytes), 支持部分失败后
     * 重试: 失败轮次不加回 hbm_used, 成功后一次性恢复, 不依赖可能被 check_and_swap_out
     * 覆盖的 swap_size */
    /* 以“本次进入循环时仍处于换出状态的记录数”为准, 而不是 check_record(check_and_swap_out
     * 可能重置它, 导致重试轮次口径不一致) */
    size_t expected_in = 0;
    for (size_t i = 0; i < hook->tracker->count; ++i) {
        if (hook->tracker->records[i].is_swapped) {
            expected_in++;
        }
    }

    hook->tracker->check_record_in = 0;
    for (size_t i = 0; i < hook->tracker->count; ++i) {
        memory_record_t *record = &(hook->tracker->records[i]);
        if (record->is_swapped == false) {
            continue;
        }
        int ret = ENPU_SUCCESS;
        uint64_t rec_size = record->size;
        if (record->is_physical) {
            ret = swap_executor_swap_in_physical(hook->executor, hook->tracker, record->ptr, record->gen,
                                                 record->swap_offset, &record->size, &record->handle);
        } else {
            ret = swap_executor_swap_in(hook->executor, &record->ptr, record->swap_offset, &record->size,
                                        &record->handle, MEMCPY_FLAG);
        }
        if (ret != ENPU_SUCCESS) {
            LOG_ERROR("swap_hook: swap_in failed for ptr=%p, ret=%d", record->ptr, ret);
            continue;
        }

        ret = memory_record_mark_active(record, record->handle);
        if (ret != ENPU_SUCCESS) {
            LOG_ERROR("swap_hook: mark_active failed for ptr=%p, ret=%d", record->ptr, ret);
            continue;
        }

        hook->tracker->pending_restore_bytes += rec_size;
        record->restore_pending = true;
        hook->tracker->check_record_in++;
        LOG_DEBUG("swap_hook: swap_in record success for ptr=%p, offset=%lu, size=%lu", record->ptr,
                  record->swap_offset, record->size);
    }

    if (hook->tracker->check_record_in != expected_in) {
        /* 有块没换入成功: 不能宣告换入完成. 否则进程带着未映射的 VA 继续下发,
         * 内核访问到无效 GM 地址会直接 507035. 保留 swapped 标志等下次重试 */
        LOG_ERROR("swap_hook: swap_in incomplete, swap_in=%zu != expected=%zu", hook->tracker->check_record_in,
                  expected_in);
        size_t leftover = 0;
        for (size_t i = 0; i < hook->tracker->count; ++i) {
            memory_record_t *record = &(hook->tracker->records[i]);
            if (!record->is_swapped) {
                continue;
            }
            if (leftover < 16) {
                LOG_WARN("swap_hook: leftover swapped record ptr=%p, gen=%lu, size=%lu, phys=%d, offset=%lu",
                         record->ptr, record->gen, record->size, (int)record->is_physical, record->swap_offset);
            }
            leftover++;
        }
        LOG_WARN("swap_hook: %zu record(s) still swapped out, retry on next launch", leftover);
        pthread_mutex_unlock(&state->swap_in_mutex);
        pthread_mutex_unlock(&hook->tracker->lock);
        return ENPU_FAIL;
    }

    /* 全部换入完成后, 把此前各轮成功恢复但未记账的字节一次性加回 */
    uint64_t used = hook->tracker->pending_restore_bytes;
    hook->tracker->pending_restore_bytes = 0;
    for (size_t i = 0; i < hook->tracker->count; ++i) {
        hook->tracker->records[i].restore_pending = false;
    }

    if (used != 0) {
        /* 进程级 + vNPU 级 hbm_used 恢复 */
        atomic_fetch_add(&my_entry->hbm_used, used);
        vnpu_shm_entry_t *vnpu_entry = shm_get_entry(state, get_vnpu_id());
        if (vnpu_entry != NULL) {
            atomic_fetch_add(&vnpu_entry->hbm_used, used);
        }
        shm_set_process_swapped(state, get_vnpu_id(), my_slot, false);

        LOG_INFO("swap_hook: swap_in completed for vnpu %d, slot %d", get_vnpu_id(), my_slot);
    }
    pthread_mutex_unlock(&state->swap_in_mutex);
    pthread_mutex_unlock(&hook->tracker->lock);
    return ENPU_SUCCESS;
}

/* 释放内存时的记账: 换出状态下 hbm_used 已由换出侧扣减, 且该记录不会再被换入,
 * 因此这里不再重复扣减; 部分换入成功但尚未加回 hbm_used 的记录同理(其待恢复量已在
 * 删除记录时从 pending_restore_bytes 核销); 其余情况按进程级 + vNPU 级扣减 hbm_used */
static void swap_hook_account_free(uint64_t size, bool was_swapped, bool restore_pending)
{
    if (was_swapped || restore_pending) {
        return;
    }

    shm_state_t *state = get_shm_state();
    vnpu_shm_entry_t *entry = state != NULL ? shm_get_entry(state, get_vnpu_id()) : NULL;
    if (entry == NULL) {
        return;
    }

    int my_slot = get_registry_slot();
    if (my_slot >= 0) {
        atomic_fetch_sub(&entry->processes[my_slot].hbm_used, size);
    }
    atomic_fetch_sub(&entry->hbm_used, size);
}

int swap_hook_free_mem(swap_hook_t *hook, void *ptr)
{
    if (hook == NULL) {
        LOG_ERROR("swap_hook_free_mem failed: hook is NULL");
        return ENPU_FAIL;
    }

    /* 原子摘除记录: 查找、字段拷贝与删除都在 tracker 锁内完成。
     * 并发删除会压缩 records[]、扩容会迁移 records[], 在锁外持有元素指针会读到无关记录 */
    memory_record_t record;
    bool found = false;
    int take_ret = memory_tracker_take(hook->tracker, ptr, &record, &found);
    CHECK_RETURN_ERROR_CODE(take_ret, "Memory tracker take record failed when free mem.");

    if (!found) {
        /* 可能是非 Device 内存, 也可能是 map key 丢失的孤儿记录: 兜底按 ptr 清一次 */
        (void)RUNTIME_HOOK_CALL(rt_library_entry, rtUnmapMem, ptr);
        (void)memory_tracker_remove(hook->tracker, ptr);
        LOG_ERROR("No record match for devPtr when free mem.");
        return ENPU_FAIL;
    }

    if (!record.is_swapped) {
        int ret = RUNTIME_HOOK_CALL(rt_library_entry, rtUnmapMem, ptr);
        if (ret != ACL_RT_SUCCESS) {
            LOG_ERROR("UnmapMem failed when free mem, record already removed anyway. ret=%d", ret);
        }
    }

    int ret = ACL_RT_SUCCESS;
    if (record.is_swapped) {
        /* 换出时已 unmap 并 rtFreePhysical, 不能再释放一次物理内存, 只回收 VA 预留 */
        ret = RUNTIME_HOOK_CALL(rt_library_entry, rtReleaseMemAddress, ptr);
        CHECK_COND_LOG_(ret != ACL_RT_SUCCESS, "Release va failed when free swapped mem.");
    } else {
        ret = RUNTIME_HOOK_CALL(rt_library_entry, rtFreePhysical, record.handle);
        if (ret == ACL_RT_SUCCESS) {
            ret = RUNTIME_HOOK_CALL(rt_library_entry, rtReleaseMemAddress, ptr);
            CHECK_COND_LOG_(ret != ACL_RT_SUCCESS, "Release va failed when free mem.");
        } else {
            LOG_ERROR("Free physical failed when free mem.");
        }
    }

    swap_hook_account_free(record.size, record.is_swapped, record.restore_pending);

    return ENPU_SUCCESS;
}

int swap_hook_free_physical_mem(swap_hook_t *hook, rtDrvMemHandle handle)
{
    if (hook == NULL) {
        LOG_ERROR("swap_hook_free_physical_mem failed: hook is NULL");
        return ENPU_FAIL;
    }

    memory_record_t record;
    bool found = false;
    int take_ret = memory_tracker_take(hook->tracker, handle, &record, &found);
    CHECK_RETURN_ERROR_CODE(take_ret, "Memory tracker take record failed when free physical mem.");

    // record不存在时, 可能是非Device内存未被记录, 也可能是 map key 丢失的孤儿记录: 兜底按 ptr 清一次
    if (!found) {
        (void)memory_tracker_remove(hook->tracker, handle);
        return RUNTIME_HOOK_CALL(rt_library_entry, rtFreePhysical, handle);
    }

    int ret = ACL_RT_SUCCESS;
    /* 换出时已 rtFreePhysical, 这里不能再释放一次物理内存 */
    if (!record.is_swapped) {
        ret = RUNTIME_HOOK_CALL(rt_library_entry, rtFreePhysical, record.handle);
        CHECK_COND_LOG_(ret != ACL_RT_SUCCESS, "Free physical failed when free physical mem.");
    }

    ret = memory_tracker_remove_handle_record(hook->tracker, handle);
    CHECK_RETURN_ERROR_CODE(ret, "Memory tracker remove handle record failed when free physical mem.");

    swap_hook_account_free(record.size, record.is_swapped, record.restore_pending);

    return ENPU_SUCCESS;
}

int swap_hook_map_mem(swap_hook_t *hook, void *devPtr, size_t size, size_t offset, rtDrvMemHandle *handle,
                      uint64_t flags)
{
    if (hook == NULL) {
        LOG_ERROR("Swap_hook_map_mem failed, hook is NULL");
        return ENPU_FAIL;
    }

    memory_record_t *record = memory_tracker_find(hook->tracker, *handle);
    // prop为NULL时, 表示之前mallocPhysical申请的内存非Device内存, 所以不被记录
    if (record == NULL) {
        return ENPU_SUCCESS;
    }

    *handle = record->handle;
    return memory_tracker_add_map_record(hook->tracker, devPtr, size, offset, record->ptr, flags, record->gen);
}

int swap_hook_unmap_mem(swap_hook_t *hook, void *devPtr)
{
    if (hook == NULL) {
        LOG_ERROR("Swap_hook_unmap_mem failed, hook is NULL");
        return ENPU_FAIL;
    }

    return memory_tracker_remove_map_record(hook->tracker, devPtr);
}

int swap_hook_global_init(int vnpu_id)
{
    if (g_global_swap_hook != NULL) {
        LOG_ERROR("swap_hook_global_init failed: already initialized");
        return ENPU_FAIL;
    }

    shm_state_t *state = get_shm_state();
    if (state == NULL) {
        LOG_ERROR("swap_hook_global_init failed: shm_state is NULL");
        return ENPU_FAIL;
    }

    /* 必须先读 generation 再读段名：EM 写 name → generation++（release），
     * 反过来读会漏掉"读 name 时为空、读 gen 时已是新值"的中间态，导致永远不再 attach。 */
    int pool_rc = pthread_mutex_lock(&state->swap_in_mutex);
    pool_rc = recover_swap_in_mutex(&state->swap_in_mutex, pool_rc);
    if (pool_rc != 0) {
        return ENPU_FAIL;
    }
    uint_fast32_t init_gen = atomic_load(&state->swap_pool_generation);

    /* ratio=0（swap_pool_name 空）不 attach，base 保持 NULL，swap 操作自然跳过 */
    if (!swap_pool_name_is_valid(state) ||
        (state->swap_pool_name[0] != '\0' &&
         swap_buffer_shm_attach_by_name(state->swap_pool_name, &g_swap_buffer) != ENPU_SUCCESS)) {
        pthread_mutex_unlock(&state->swap_in_mutex);
        return ENPU_FAIL;
    }
    /* 记录看到段名时的 generation；若期间 EM 又 bump，下一次 reload 会重挂 */
    atomic_store(&g_pool_generation, init_gen);
    pthread_mutex_unlock(&state->swap_in_mutex);

    swap_executor_t *executor = swap_executor_create_ref(&g_swap_buffer.base);
    if (executor == NULL) {
        LOG_ERROR("swap_hook_global_init failed: swap_executor_create returned NULL");
        swap_buffer_shm_detach(&g_swap_buffer);
        return ENPU_FAIL;
    }

    memory_tracker_t *tracker = memory_tracker_create(300);
    if (tracker == NULL) {
        LOG_ERROR("swap_hook_global_init failed: memory_tracker_create returned NULL");
        swap_executor_destroy(executor);
        swap_buffer_shm_detach(&g_swap_buffer);
        return ENPU_FAIL;
    }

    g_global_swap_hook = swap_hook_create(vnpu_id, executor, tracker);
    if (g_global_swap_hook == NULL) {
        LOG_ERROR("swap_hook_global_init failed: create returned NULL");
        swap_executor_destroy(executor);
        memory_tracker_destroy(tracker);
        swap_buffer_shm_detach(&g_swap_buffer);
        return ENPU_FAIL;
    }

    vnpu_shm_entry_t *entry = shm_get_entry(state, vnpu_id);
    // request=limit时，不做显存超分，不启动监控线程
    if (entry != NULL && get_swap_enabled()) {
        swap_monitor_thread_t **monitor_thread = swap_monitor_get_thread();
        *monitor_thread = swap_monitor_thread_create(vnpu_id, state, executor, tracker, 10);
        if (*monitor_thread == NULL) {
            LOG_WARN("swap_hook_global_init: swap_monitor_thread_create failed, swap monitoring disabled");
            /* 无 monitor 线程的进程无法响应换出命令, 若仍留在注册表中会永久占用
             * swap_cmd.refcount 的递减份额, 使换出完成判定永远无法置位; 此处注销本进程 */
            int slot = get_registry_slot();
            if (slot >= 0) {
                unregister_from_shm_state(state, vnpu_id, slot);
            }
        }
    }

    LOG_INFO("swap_hook_global_init: initialized for vnpu_id=%d", vnpu_id);
    return ENPU_SUCCESS;
}

void swap_hook_global_destroy(void)
{
    swap_monitor_thread_t **monitor_thread = swap_monitor_get_thread();
    if (*monitor_thread != NULL) {
        swap_monitor_thread_stop(*monitor_thread);
        swap_monitor_thread_destroy(*monitor_thread);
        *monitor_thread = NULL;
    }

    if (g_global_swap_hook != NULL) {
        swap_hook_destroy(g_global_swap_hook);
        g_global_swap_hook = NULL;
    }

    swap_buffer_shm_detach(&g_swap_buffer);

    int slot = get_registry_slot();
    if (slot >= 0) {
        unregister_from_shm_state(get_shm_state(), get_vnpu_id(), slot);
    }
}

swap_hook_t *swap_hook_get_global(void)
{
    return g_global_swap_hook;
}

swap_pool_reload_result_t swap_hook_reload_pool(void)
{
    shm_state_t *state = get_shm_state();
    if (state == NULL) {
        return SWAP_POOL_FAILED;
    }

    uint_fast32_t cur_gen = atomic_load(&state->swap_pool_generation);
    if (cur_gen == atomic_load(&g_pool_generation)) {
        return g_swap_buffer.base != NULL ? SWAP_POOL_READY : SWAP_POOL_DISABLED;
    }

    /* 与 swap_in 互斥：swap_in 持 swap_in_mutex，避免 munmap 与 in-flight memcpy 冲突 */
    /* 持有换入锁的线程可能正在等待当前 monitor 的 ACK，此处不能阻塞。 */
    int rc = pthread_mutex_trylock(&state->swap_in_mutex);
    if (rc == EBUSY) {
        return SWAP_POOL_DEFERRED;
    }
    rc = recover_swap_in_mutex(&state->swap_in_mutex, rc);
    if (rc != 0) {
        LOG_ERROR("swap_hook_reload_pool: lock swap_in_mutex failed, ret=%d", rc);
        return SWAP_POOL_FAILED;
    }

    cur_gen = atomic_load(&state->swap_pool_generation);
    if (cur_gen == atomic_load(&g_pool_generation)) {
        pthread_mutex_unlock(&state->swap_in_mutex);
        return g_swap_buffer.base != NULL ? SWAP_POOL_READY : SWAP_POOL_DISABLED;
    }

    /* 先挂新池；失败时保留旧映射和代次，下次轮询重试。空池名表示关闭超分。 */
    swap_buffer_mapping_t next = {0};
    if (!swap_pool_name_is_valid(state) ||
        (state->swap_pool_name[0] != '\0' &&
         swap_buffer_shm_try_attach_by_name(state->swap_pool_name, &next) != ENPU_SUCCESS)) {
        pthread_mutex_unlock(&state->swap_in_mutex);
        return SWAP_POOL_FAILED;
    }
    if (swap_buffer_shm_detach(&g_swap_buffer) != ENPU_SUCCESS) {
        swap_buffer_shm_detach(&next);
        pthread_mutex_unlock(&state->swap_in_mutex);
        return SWAP_POOL_FAILED;
    }
    g_swap_buffer = next;
    atomic_store(&g_pool_generation, cur_gen);
    pthread_mutex_unlock(&state->swap_in_mutex);
    return next.base != NULL ? SWAP_POOL_READY : SWAP_POOL_DISABLED;
}
