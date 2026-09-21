/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 */

#include "swap_executor.h"
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "npu_manager.h"

// rtMemcpyKind_t
#define RT_MEMCPY_HOST_TO_DEVICE 1
#define RT_MEMCPY_DEVICE_TO_HOST 2
#define NORMAL_PAGE_TYPE 0U
#define HBM_TYPE 0U

struct swap_executor {
    void *swap_buffer_base;
};

swap_executor_t *swap_executor_create(void *swap_buffer_base)
{
    if (swap_buffer_base == NULL) {
        return NULL;
    }

    swap_executor_t *executor = (swap_executor_t *)calloc(1, sizeof(swap_executor_t));
    if (executor == NULL) {
        return NULL;
    }

    executor->swap_buffer_base = swap_buffer_base;

    return executor;
}

int swap_executor_destroy(swap_executor_t *executor)
{
    if (executor == NULL) {
        return ENPU_FAIL;
    }

    free(executor);
    return ENPU_SUCCESS;
}

int swap_executor_swap_out(swap_executor_t *executor, memory_tracker_t *tracker, memory_record_t **records, int count,
                           uint64_t swap_offset, uint64_t *act_swapped)
{
    if (executor == NULL || records == NULL || count <= 0) {
        return ENPU_FAIL;
    }

    uint64_t current_offset = swap_offset;
    tracker->check_record = 0;

    for (int i = 0; i < count; i++) {
        memory_record_t *record = records[i];
        if (record == NULL || record->ptr == NULL || record->is_swapped) {
            continue;
        }

        void *dst = (void *)((uintptr_t)executor->swap_buffer_base + current_offset);
        if (record->is_physical) {
            void *va;
            int rc = memory_tracker_get_map_va(tracker, record->ptr, &va);
            bool flag = false;
            if (rc != ENPU_SUCCESS || va == NULL) {
                rc = RUNTIME_HOOK_CALL(rt_library_entry, rtReserveMemAddress, &va, record->size, 0, NULL, 0);
                if (rc != RT_ERROR_NONE) {
                    LOG_INFO("Alloc va for physical mem failed.");
                    continue;
                }
                rc = RUNTIME_HOOK_CALL(rt_library_entry, rtMapMem, va, record->size, 0, record->handle, 0);
                if (rc != RT_ERROR_NONE) {
                    LOG_INFO("Map va and physical mem failed.");
                    continue;
                }
                flag = true;
            }
            rc = RUNTIME_HOOK_CALL(rt_library_entry, rtMemcpy, dst, record->size, va, record->size,
                                   RT_MEMCPY_DEVICE_TO_HOST);
            if (rc != RT_ERROR_NONE) {
                LOG_INFO("swap_executor_swap_out: rtMemcpy failed for record=%p, ret=%d, swap_offset=%lu", record, rc,
                         current_offset);
                continue;
            }
            rc = RUNTIME_HOOK_CALL(rt_library_entry, rtUnmapMem, va);
            if (rc != RT_ERROR_NONE) {
                LOG_INFO("Unmap va and physical mem failed.");
                continue;
            }
            if (flag) {
                rc = RUNTIME_HOOK_CALL(rt_library_entry, rtReleaseMemAddress, va);
                if (rc != RT_ERROR_NONE) {
                    LOG_INFO("Release va for physical mem failed.");
                    continue;
                }
            }
        } else {
            int ret = RUNTIME_HOOK_CALL(rt_library_entry, rtMemcpy, dst, record->size, record->ptr, record->size,
                                        RT_MEMCPY_DEVICE_TO_HOST);
            if (ret != RT_ERROR_NONE) {
                LOG_INFO("swap_executor_swap_out: rtMemcpy failed for record=%p, ret=%d, swap_offset=%lu", record, ret,
                         current_offset);
                continue;
            }
            ret = RUNTIME_HOOK_CALL(rt_library_entry, rtUnmapMem, record->ptr);
            if (ret != RT_ERROR_NONE) {
                LOG_INFO("swap_executor_swap_out: rtUnmapMem failed for record=%p, ret=%d", record, ret);
                continue;
            }
        }
        int ret = RUNTIME_HOOK_CALL(rt_library_entry, rtFreePhysical, record->handle);
        if (ret != RT_ERROR_NONE) {
            LOG_INFO("swap_executor_swap_out: rtFreePhysical failed for record=%p, ret=%d", record, ret);
            continue;
            ;
        }

        memory_record_mark_swapped(record, current_offset);
        current_offset += record->size;
        *act_swapped += record->size;
        tracker->check_record++;
    }

    if (tracker->check_record != (size_t)count) {
        LOG_WARN("swap_executor_swap_out: check_record=%zu != expected count=%d", tracker->check_record, count);
    }

    return ENPU_SUCCESS;
}

int swap_executor_swap_in(swap_executor_t *executor, void **ptr, uint64_t offset, uint64_t *size,
                          rtDrvMemHandle *new_handle, uint8_t flag)
{
    if (executor == NULL || ptr == NULL || size == 0 || new_handle == NULL) {
        return ENPU_FAIL;
    }

    rtDrvMemProp_t rtProp = {};
    rtProp.side = ACL_MEM_LOCATION_TYPE_DEVICE;
    rtProp.devid = get_device_id();
    rtProp.module_id = APP_MODE_ID_U16;
    rtProp.pg_type = NORMAL_PAGE_TYPE;
    rtProp.mem_type = HBM_TYPE;

    size_t granularity = 0UL;
    int ret = RUNTIME_HOOK_CALL(rt_library_entry, rtMemGetAllocationGranularity, &rtProp, 0, &granularity);
    CHECK_COND_RETURN_ERROR_CODE(ret != ACL_RT_SUCCESS, "Get granularity failed.");
    // 根据查询到的内存申请粒度做内存对齐, 以便节约内存
    size_t alignedSize = ((*size + granularity - 1U) / granularity) * granularity;

    ret = RUNTIME_HOOK_CALL(rt_library_entry, rtMallocPhysical, new_handle, alignedSize, &rtProp, 0);
    CHECK_COND_RETURN_ERROR_CODE(ret != ACL_RT_SUCCESS, "Malloc physical failed.");

    if (flag == NOT_MEMCPY_FLAG) {
        ret = RUNTIME_HOOK_CALL(rt_library_entry, rtReserveMemAddress, ptr, alignedSize, 0, NULL, 0);
        CHECK_COND_RETURN_(ret != ACL_RT_SUCCESS, ret, "Malloc va failed.");
    }

    ret = RUNTIME_HOOK_CALL(rt_library_entry, rtMapMem, *ptr, alignedSize, 0, *new_handle, 0);
    if (ret != ACL_RT_SUCCESS) {
        LOG_ERROR("Map mem failed.");
        ret = RUNTIME_HOOK_CALL(rt_library_entry, rtFreePhysical, *new_handle);
        CHECK_COND_RETURN_ERROR_CODE(ret != ACL_RT_SUCCESS, "Free physical failed.");
        return ENPU_FAIL;
    }

    *size = alignedSize;

    if (flag == NOT_MEMCPY_FLAG) {
        return ENPU_SUCCESS;
    }
    void *src = (void *)((uintptr_t)executor->swap_buffer_base + offset);
    ret = RUNTIME_HOOK_CALL(rt_library_entry, rtMemcpy, *ptr, alignedSize, src, alignedSize, RT_MEMCPY_HOST_TO_DEVICE);
    if (ret != ACL_RT_SUCCESS) {
        LOG_ERROR("Swap in memcpy failed.");
        ret = RUNTIME_HOOK_CALL(rt_library_entry, rtFreePhysical, *new_handle);
        CHECK_COND_RETURN_ERROR_CODE(ret != ACL_RT_SUCCESS, "Free physical failed.");
        return ENPU_FAIL;
    }

    return ENPU_SUCCESS;
}

int swap_executor_swap_in_physical(swap_executor_t *executor, memory_tracker_t *tracker, rtDrvMemHandle handle,
                                   uint64_t offset, uint64_t *size, rtDrvMemHandle *new_handle)
{
    if (executor == NULL || handle == NULL) {
        return ENPU_FAIL;
    }

    rtDrvMemProp_t rtProp = {};
    uint64_t flags = 0;
    int ret = memory_tracker_get_handle_prop(tracker, handle, size, &rtProp, &flags);
    if (ret != ENPU_SUCCESS) {
        LOG_INFO("Get handle prop failed.");
        rtProp.side = ACL_MEM_LOCATION_TYPE_DEVICE;
        rtProp.devid = get_device_id();
        rtProp.module_id = APP_MODE_ID_U16;
        rtProp.pg_type = NORMAL_PAGE_TYPE;
        rtProp.mem_type = HBM_TYPE;
    }
    ret = RUNTIME_HOOK_CALL(rt_library_entry, rtMallocPhysical, new_handle, *size, &rtProp, flags);
    CHECK_COND_RETURN_ERROR_CODE(ret != ACL_RT_SUCCESS, "Malloc physical failed.");

    void *va;
    ret = memory_tracker_get_map_va(tracker, handle, &va);
    bool flag = false;
    if (ret != ENPU_SUCCESS || va == NULL) {
        ret = RUNTIME_HOOK_CALL(rt_library_entry, rtReserveMemAddress, &va, *size, 0, NULL, 0);
        CHECK_COND_RETURN_ERROR_CODE(ret != ACL_RT_SUCCESS, "Alloc va failed.");
        flag = true;
        ret = RUNTIME_HOOK_CALL(rt_library_entry, rtMapMem, va, *size, 0, *new_handle, 0);
    } else {
        size_t map_size = 0;
        size_t map_offset = 0;
        uint64_t map_flags = 0;
        ret = memory_tracker_get_map_prop(tracker, va, &map_size, &map_offset, &map_flags);
        if (ret == ENPU_SUCCESS) {
            ret = RUNTIME_HOOK_CALL(rt_library_entry, rtMapMem, va, map_size, map_offset, *new_handle, map_flags);
        } else {
            ret = RUNTIME_HOOK_CALL(rt_library_entry, rtMapMem, va, *size, 0, *new_handle, 0);
        }
    }
    if (ret != ACL_RT_SUCCESS) {
        LOG_ERROR("Map mem failed.");
        ret = RUNTIME_HOOK_CALL(rt_library_entry, rtFreePhysical, *new_handle);
        CHECK_COND_RETURN_ERROR_CODE(ret != ACL_RT_SUCCESS, "Free physical failed.");
        return ENPU_FAIL;
    }

    void *src = (void *)((uintptr_t)executor->swap_buffer_base + offset);
    ret = RUNTIME_HOOK_CALL(rt_library_entry, rtMemcpy, va, *size, src, *size, RT_MEMCPY_HOST_TO_DEVICE);
    if (ret != ACL_RT_SUCCESS) {
        LOG_ERROR("Swap in memcpy failed.");
        ret = RUNTIME_HOOK_CALL(rt_library_entry, rtFreePhysical, *new_handle);
        CHECK_COND_RETURN_ERROR_CODE(ret != ACL_RT_SUCCESS, "Free physical failed.");
        return ENPU_FAIL;
    }

    if (flag) {
        ret = RUNTIME_HOOK_CALL(rt_library_entry, rtUnmapMem, va);
        CHECK_COND_RETURN_ERROR_CODE(ret != ACL_RT_SUCCESS, "Unmap va and pa failed.");
        ret = RUNTIME_HOOK_CALL(rt_library_entry, rtReleaseMemAddress, va);
        CHECK_COND_RETURN_ERROR_CODE(ret != ACL_RT_SUCCESS, "Release va failed.");
    }

    return ENPU_SUCCESS;
}