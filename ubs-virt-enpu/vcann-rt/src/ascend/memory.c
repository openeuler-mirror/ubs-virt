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

#include "core_limiter.h"
#include "log.h"
#include "mem_limiter.h"
#include "npu_manager.h"
#include "runtime_hook.h"
#include "swap_hook.h"

void load_rt_libraries(void);

RUNTIME_HOOK_DEFINE(rtMalloc, void **devPtr, uint64_t size, rtMemType_t type, const uint16_t moduleId)
{
    LOG_DEBUG("Hook mem rtMalloc size:%" PRIu64 ".", size);
    swap_hook_t *swap_hook = swap_hook_get_global();
    int ret = guard_memory(size, swap_hook != NULL);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    if (swap_hook == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, rtMalloc, devPtr, size, type, moduleId);
    }

    ret = swap_hook_check_and_swap_in(swap_hook);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook check and swap in failed when rtMalloc.");
    ret = swap_hook_malloc_mem(swap_hook, devPtr, size);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook malloc failed.");

    return ACL_RT_SUCCESS;
}

RUNTIME_HOOK_DEFINE(aclrtMallocImpl, void **devPtr, size_t size, aclrtMemMallocPolicy policy)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMallocImpl, devPtr, size, policy);
    }
    LOG_DEBUG("Hook mem aclrtMallocImpl size:%" PRIu64 ".", size);
    swap_hook_t *swap_hook = swap_hook_get_global();
    int ret = guard_memory(size, swap_hook != NULL);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    if (swap_hook == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMallocImpl, devPtr, size, policy);
    }

    ret = swap_hook_check_and_swap_in(swap_hook);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook check and swap in failed when aclrtMalloc.");
    ret = swap_hook_malloc_mem(swap_hook, devPtr, size);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook aclrtMalloc failed.");

    return ACL_RT_SUCCESS;
}

RUNTIME_HOOK_DEFINE(aclrtMallocAlign32Impl, void **devPtr, size_t size, aclrtMemMallocPolicy policy)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMallocAlign32Impl, devPtr, size, policy);
    }
    LOG_DEBUG("Hook mem aclrtMallocAlign32Impl size:%" PRIu64 ".", size);
    int ret = guard_memory(size, swap_hook_get_global() != NULL);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMallocAlign32Impl, devPtr, size, policy);
}

RUNTIME_HOOK_DEFINE(rtMallocCached, void **devPtr, uint64_t size, rtMemType_t type, const uint16_t moduleId)
{
    LOG_DEBUG("Hook mem rtMallocCached size:%" PRIu64 ".", size);
    int ret = guard_memory(size, swap_hook_get_global() != NULL);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtMallocCached, devPtr, size, type, moduleId);
}

RUNTIME_HOOK_DEFINE(aclrtMallocCachedImpl, void **devPtr, size_t size, aclrtMemMallocPolicy policy)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMallocCachedImpl, devPtr, size, policy);
    }
    LOG_DEBUG("Hook mem aclrtMallocCachedImpl size:%" PRIu64 ".", size);
    int ret = guard_memory(size, swap_hook_get_global() != NULL);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMallocCachedImpl, devPtr, size, policy);
}

RUNTIME_HOOK_DEFINE(rtDvppMalloc, void **devPtr, uint64_t size, uint16_t moduleId)
{
    LOG_DEBUG("Hook mem rtDvppMalloc size:%" PRIu64 ".", size);
    int ret = guard_memory(size, swap_hook_get_global() != NULL);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtDvppMalloc, devPtr, size, moduleId);
}

RUNTIME_HOOK_DEFINE(rtDvppMallocWithFlag, void **devPtr, uint64_t size, uint32_t flag, const uint16_t moduleId)
{
    LOG_DEBUG("Hook mem rtDvppMallocWithFlag size:%" PRIu64 ".", size);
    int ret = guard_memory(size, swap_hook_get_global() != NULL);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtDvppMallocWithFlag, devPtr, size, flag, moduleId);
}

RUNTIME_HOOK_DEFINE(rtMemAlloc, void **devPtr, uint64_t size, rtMallocPolicy policy, rtMallocAdvise advise,
                    rtMallocConfig_t *cfg)
{
    LOG_DEBUG("Hook mem rtMemAlloc size:%" PRIu64 ".", size);
    int ret = guard_memory(size, swap_hook_get_global() != NULL);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemAlloc, devPtr, size, policy, advise, cfg);
}

RUNTIME_HOOK_DEFINE(rtMemAllocManaged, void **ptr, uint64_t size, uint32_t flag, const uint16_t moduleId)
{
    LOG_DEBUG("Hook mem rtMemAllocManaged size:%" PRIu64 ".", size);
    int ret = guard_memory(size, swap_hook_get_global() != NULL);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemAllocManaged, ptr, size, flag, moduleId);
}

RUNTIME_HOOK_DEFINE(aclrtMemAllocManagedImpl, void **ptr, uint64_t size, uint32_t flag)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemAllocManagedImpl, ptr, size, flag);
    }
    LOG_DEBUG("Hook mem aclrtMemAllocManagedImpl size:%" PRIu64 ".", size);
    int ret = guard_memory(size, swap_hook_get_global() != NULL);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemAllocManagedImpl, ptr, size, flag);
}

RUNTIME_HOOK_DEFINE(rtMallocPhysical, rtDrvMemHandle *handle, size_t size, rtDrvMemProp_t *prop, uint64_t flags)
{
    LOG_DEBUG("Hook mem rtMallocPhysical size:%zd.", size);
    if (prop == NULL || prop->side != ACL_MEM_LOCATION_TYPE_DEVICE) {
        return RUNTIME_HOOK_CALL(rt_library_entry, rtMallocPhysical, handle, size, prop, flags);
    }

    swap_hook_t *swap_hook = swap_hook_get_global();
    int ret = guard_memory(size, swap_hook != NULL);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    if (swap_hook == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, rtMallocPhysical, handle, size, prop, flags);
    }

    ret = swap_hook_check_and_swap_in(swap_hook);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook check and swap in failed when rtMallocPhysical.");
    ret = RUNTIME_HOOK_CALL(rt_library_entry, rtMallocPhysical, handle, size, prop, flags);
    if (ret != ACL_RT_SUCCESS) {
        return ret;
    }
    ret = swap_hook_malloc_physical_mem(swap_hook, *handle, size, prop, flags);
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("Swap hook rtMallocPhysical failed, ret:%d.", ret);
        int rc = RUNTIME_HOOK_CALL(rt_library_entry, rtFreePhysical, *handle);
        CHECK_RETURN_ERROR_CODE(rc, "rtfreePhysical failed after malloc physical fail.");
        return ret;
    }

    return ACL_RT_SUCCESS;
}

/* 参考 runtime aclrtMallocPhysicalImpl 的转换: aclrtPhysicalMemProp -> rtDrvMemProp_t */
static void aclrt_prop_to_rt_prop(const aclrtPhysicalMemProp *prop, rtDrvMemProp_t *rtProp)
{
    rtProp->side = prop->location.type;
    rtProp->devid = prop->location.id;
    rtProp->module_id = APP_MODE_ID_U16;
    rtProp->reserve = prop->reserve;
    rtProp->pg_type = NORMAL_PAGE_TYPE;
    rtProp->mem_type = HBM_TYPE;

    switch (prop->memAttr) {
        case ACL_HBM_MEM_HUGE:
        case ACL_MEM_HUGE:
            rtProp->pg_type = HUGE_PAGE_TYPE;
            break;
        case ACL_HBM_MEM_HUGE1G:
        case ACL_MEM_HUGE1G:
            rtProp->pg_type = HUGE1G_PAGE_TYPE;
            break;
        case ACL_MEM_P2P_NORMAL:
            rtProp->mem_type = P2P_HBM_TYPE;
            break;
        case ACL_MEM_P2P_HUGE:
            rtProp->pg_type = HUGE_PAGE_TYPE;
            rtProp->mem_type = P2P_HBM_TYPE;
            break;
        case ACL_MEM_P2P_HUGE1G:
            rtProp->pg_type = HUGE1G_PAGE_TYPE;
            rtProp->mem_type = P2P_HBM_TYPE;
            break;
        case ACL_HBM_MEM_P2P_NORMAL:
            rtProp->mem_type = P2P_HBM_TYPE;
            break;
        case ACL_HBM_MEM_P2P_HUGE:
            rtProp->pg_type = HUGE_PAGE_TYPE;
            rtProp->mem_type = P2P_HBM_TYPE;
            break;
        case ACL_HBM_MEM_P2P_HUGE1G:
            rtProp->pg_type = HUGE1G_PAGE_TYPE;
            rtProp->mem_type = P2P_HBM_TYPE;
            break;
        default:
            break;
    }
}

RUNTIME_HOOK_DEFINE(aclrtMallocPhysicalImpl, aclrtDrvMemHandle *handle, size_t size, const aclrtPhysicalMemProp *prop,
                    uint64_t flags)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMallocPhysicalImpl, handle, size, prop, flags);
    }
    LOG_DEBUG("Hook mem aclrtMallocPhysicalImpl size:%zd.", size);
    if (prop == NULL || prop->location.type != ACL_MEM_LOCATION_TYPE_DEVICE) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMallocPhysicalImpl, handle, size, prop, flags);
    }

    swap_hook_t *swap_hook = swap_hook_get_global();
    int ret = guard_memory(size, swap_hook != NULL);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    if (swap_hook == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMallocPhysicalImpl, handle, size, prop, flags);
    }

    ret = swap_hook_check_and_swap_in(swap_hook);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook check and swap in failed when aclrtMallocPhysical.");
    ret = RUNTIME_HOOK_CALL(rt_library_entry, aclrtMallocPhysicalImpl, handle, size, prop, flags);
    if (ret != ACL_RT_SUCCESS) {
        return ret;
    }
    rtDrvMemProp_t rtProp = {};
    aclrt_prop_to_rt_prop(prop, &rtProp);
    ret = swap_hook_malloc_physical_mem(swap_hook, *handle, size, &rtProp, flags);
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("Swap hook aclrtMallocPhysical failed, ret:%d.", ret);
        int rc = RUNTIME_HOOK_CALL(rt_library_entry, aclrtFreePhysicalImpl, *handle);
        CHECK_RETURN_ERROR_CODE(rc, "aclrtfreePhysicalImpl failed after malloc physical fail.");
        return ret;
    }

    return ACL_RT_SUCCESS;
}

RUNTIME_HOOK_DEFINE(rtMemGetInfoEx, rtMemInfoType_t memInfoType, size_t *freeSize, size_t *totalSize)
{
    (void)memInfoType;
    int res = log_init();
    CHECK_COND_RETURN_((res != ENPU_SUCCESS), res, "Failed to init log module, res:%d.", res);
    pre_rt_init();
    enpu_global_init();
    CHECK_COND_RETURN_(!check_init_success(), ACL_ERROR_UNINITIALIZE,
                       "Failed to initialize vcann-rt, please check the config file in %s.", NPU_CONFIG_PATH);
    LOG_DEBUG("Hook mem rtMemGetInfoEx.");
    if (freeSize == NULL || totalSize == NULL) {
        LOG_ERROR("rtMemGetInfoEx: freeSize or totalSize is NULL.");
        return RT_ERROR_INVALID_VALUE;
    }
    size_t quota = get_mem_limit_quota();
    size_t used;
    int ret = get_mem_used(&used);
    if (ret != 0) {
        LOG_ERROR("Get mem used failed.");
        return RT_ERROR_INVALID_VALUE;
    }
    if (used > quota) {
        LOG_ERROR("Mem used is abnormally high, exceeding the quota.");
        return RT_ERROR_INVALID_VALUE;
    }
    /* 已换出到主机内存的部分仍占用本 vNPU 的 limit 口径: 换出释放的 HBM 供其他 vNPU
     * 弹性借用, 本 vNPU 剩余额度必须扣除已换出量, 防止换出后超 limit 申请新显存.
     * 换入过程中实际占用逐步回升, 逻辑用量可能瞬时超过 limit, 此时按剩余 0 处理 */
    size_t swapped = (size_t)get_vnpu_swapped_size();
    size_t logical = (used > SIZE_MAX - swapped) ? SIZE_MAX : used + swapped;
    size_t remain = (logical >= quota) ? 0 : quota - logical;
    if (get_shm_state() == NULL || quota == get_mem_request_quota()) {
        *freeSize = remain;
        *totalSize = quota;
        return RT_ERROR_NONE;
    }

    size_t dynamic_free = get_mem_dynamic_free();
    size_t request = get_mem_request_quota();
    if (request > logical) {
        size_t free_mem_1 = request - logical + dynamic_free;
        size_t free_mem_2 = (free_mem_1 < remain) ? free_mem_1 : remain;
        *freeSize = (free_mem_2 > request - logical) ? free_mem_2 : request - logical;
    } else {
        *freeSize = (dynamic_free < remain) ? dynamic_free : remain;
    }
    *totalSize = quota;
    return RT_ERROR_NONE;
}

RUNTIME_HOOK_DEFINE(aclrtGetMemInfoImpl, aclrtMemAttr attr, size_t *free, size_t *total)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtGetMemInfoImpl, attr, free, total);
    }
    (void)attr;
    int res = log_init();
    CHECK_COND_RETURN_((res != ENPU_SUCCESS), res, "Failed to init log module, res:%d.", res);
    pre_rt_init();
    enpu_global_init();
    CHECK_COND_RETURN_(!check_init_success(), ACL_ERROR_UNINITIALIZE,
                       "Failed to initialize vcann-rt, please check the config file in %s.", NPU_CONFIG_PATH);
    LOG_DEBUG("Hook mem aclrtGetMemInfoImpl.");
    if (free == NULL || total == NULL) {
        LOG_ERROR("aclrtGetMemInfoImpl: free or total is NULL.");
        return RT_ERROR_INVALID_VALUE;
    }
    size_t quota = get_mem_limit_quota();
    size_t used;
    int ret = get_mem_used(&used);
    if (ret != 0) {
        LOG_ERROR("Get mem used failed.");
        return RT_ERROR_INVALID_VALUE;
    }
    if (used > quota) {
        LOG_ERROR("Mem used is abnormally high, exceeding the quota.");
        return RT_ERROR_INVALID_VALUE;
    }
    /* 已换出到主机内存的部分仍占用本 vNPU 的 limit 口径, 防止换出后超 limit 申请新显存;
     * 换入过程中逻辑用量可能瞬时超过 limit, 此时按剩余 0 处理 */
    size_t swapped = (size_t)get_vnpu_swapped_size();
    size_t logical = (used > SIZE_MAX - swapped) ? SIZE_MAX : used + swapped;
    size_t remain = (logical >= quota) ? 0 : quota - logical;
    if (get_shm_state() == NULL || quota == get_mem_request_quota()) {
        *free = remain;
        *total = quota;
        return ACL_RT_SUCCESS;
    }

    size_t dynamic_free = get_mem_dynamic_free();
    size_t request = get_mem_request_quota();
    if (request > logical) {
        size_t free_mem_1 = request - logical + dynamic_free;
        size_t free_mem_2 = (free_mem_1 < remain) ? free_mem_1 : remain;
        *free = (free_mem_2 > request - logical) ? free_mem_2 : request - logical;
    } else {
        *free = (dynamic_free < remain) ? dynamic_free : remain;
    }
    *total = quota;
    return ACL_RT_SUCCESS;
}
/* mem-swap 扩展: 显存换入换出所需内存接口拦截, 初始化序列与上方 master 版 hook 保持一致 */
RUNTIME_HOOK_DEFINE(rtFree, void *devPtr)
{
    LOG_DEBUG("Hook mem rtFree.");
    if (get_shm_state() == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, rtFree, devPtr);
    }

    swap_hook_t *swap_hook = swap_hook_get_global();
    if (swap_hook == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, rtFree, devPtr);
    }
    int ret = swap_hook_check_and_swap_in(swap_hook);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook check and swap in failed when rtFree.");
    ret = swap_hook_free_mem(swap_hook, devPtr);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook free mem failed when rtFree.");

    return ACL_RT_SUCCESS;
}

RUNTIME_HOOK_DEFINE(aclrtFreeImpl, void *devPtr)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtFreeImpl, devPtr);
    }
    LOG_DEBUG("Hook mem aclrtFreeImpl.");
    if (get_shm_state() == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtFreeImpl, devPtr);
    }

    swap_hook_t *swap_hook = swap_hook_get_global();
    if (swap_hook == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtFreeImpl, devPtr);
    }
    int ret = swap_hook_check_and_swap_in(swap_hook);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook check and swap in failed when aclrtFreeImpl.");
    ret = swap_hook_free_mem(swap_hook, devPtr);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook free mem failed when aclrtFreeImpl.");

    return ACL_RT_SUCCESS;
}

RUNTIME_HOOK_DEFINE(rtFreePhysical, rtDrvMemHandle handle)
{
    LOG_DEBUG("Hook mem rtFreePhysical.");
    if (get_shm_state() == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, rtFreePhysical, handle);
    }

    swap_hook_t *swap_hook = swap_hook_get_global();
    if (swap_hook == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, rtFreePhysical, handle);
    }
    int ret = swap_hook_check_and_swap_in(swap_hook);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook check and swap in failed when rtFreePhysical.");
    ret = swap_hook_free_physical_mem(swap_hook, handle);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook free mem failed when rtFreePhysical.");

    return ACL_RT_SUCCESS;
}

RUNTIME_HOOK_DEFINE(aclrtFreePhysicalImpl, aclrtDrvMemHandle handle)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtFreePhysicalImpl, handle);
    }
    LOG_DEBUG("Hook mem aclrtFreePhysicalImpl.");
    if (get_shm_state() == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtFreePhysicalImpl, handle);
    }

    swap_hook_t *swap_hook = swap_hook_get_global();
    if (swap_hook == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtFreePhysicalImpl, handle);
    }
    int ret = swap_hook_check_and_swap_in(swap_hook);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook check and swap in failed when aclrtFreePhysicalImpl.");
    ret = swap_hook_free_physical_mem(swap_hook, handle);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook free mem failed when aclrtFreePhysicalImpl.");

    return ACL_RT_SUCCESS;
}

RUNTIME_HOOK_DEFINE(rtMapMem, void *devPtr, size_t size, size_t offset, rtDrvMemHandle handle, uint64_t flags)
{
    LOG_DEBUG("Hook mem rtMapMem.");
    if (get_shm_state() == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, rtMapMem, devPtr, size, offset, handle, flags);
    }

    swap_hook_t *swap_hook = swap_hook_get_global();
    if (swap_hook == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, rtMapMem, devPtr, size, offset, handle, flags);
    }
    int ret = swap_hook_check_and_swap_in(swap_hook);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook check and swap in failed when rtMapMem.");
    ret = swap_hook_map_mem(swap_hook, devPtr, size, offset, &handle, flags);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook map mem failed when rtMapMem.");
    return RUNTIME_HOOK_CALL(rt_library_entry, rtMapMem, devPtr, size, offset, handle, flags);
}

RUNTIME_HOOK_DEFINE(aclrtMapMemImpl, void *virPtr, size_t size, size_t offset, aclrtDrvMemHandle handle, uint64_t flags)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMapMemImpl, virPtr, size, offset, handle, flags);
    }
    LOG_DEBUG("Hook mem aclrtMapMemImpl.");
    if (get_shm_state() == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMapMemImpl, virPtr, size, offset, handle, flags);
    }

    swap_hook_t *swap_hook = swap_hook_get_global();
    if (swap_hook == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMapMemImpl, virPtr, size, offset, handle, flags);
    }
    int ret = swap_hook_check_and_swap_in(swap_hook);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook check and swap in failed when aclrtMapMemImpl.");
    ret = swap_hook_map_mem(swap_hook, virPtr, size, offset, &handle, flags);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook map mem failed when aclrtMapMemImpl.");
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMapMemImpl, virPtr, size, offset, handle, flags);
}

RUNTIME_HOOK_DEFINE(rtUnmapMem, void *devPtr)
{
    LOG_DEBUG("Hook mem rtUnmapMem.");
    if (get_shm_state() == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, rtUnmapMem, devPtr);
    }

    swap_hook_t *swap_hook = swap_hook_get_global();
    if (swap_hook == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, rtUnmapMem, devPtr);
    }
    int ret = swap_hook_check_and_swap_in(swap_hook);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook check and swap in failed when rtUnmapMem.");
    ret = swap_hook_unmap_mem(swap_hook, devPtr);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook unmap mem failed when rtUnmapMem.");
    return RUNTIME_HOOK_CALL(rt_library_entry, rtUnmapMem, devPtr);
}

RUNTIME_HOOK_DEFINE(aclrtUnmapMemImpl, void *devPtr)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtUnmapMemImpl, devPtr);
    }
    LOG_DEBUG("Hook mem aclrtUnmapMemImpl.");
    if (get_shm_state() == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtUnmapMemImpl, devPtr);
    }

    swap_hook_t *swap_hook = swap_hook_get_global();
    if (swap_hook == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtUnmapMemImpl, devPtr);
    }
    int ret = swap_hook_check_and_swap_in(swap_hook);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook check and swap in failed when aclrtUnmapMemImpl.");
    ret = swap_hook_unmap_mem(swap_hook, devPtr);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook unmap mem failed when aclrtUnmapMemImpl.");
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtUnmapMemImpl, devPtr);
}

RUNTIME_HOOK_DEFINE(rtMemcpy, void *dst, uint64_t destMax, const void *src, uint64_t cnt, rtMemcpyKind_t kind)
{
    LOG_DEBUG("Hook mem rtMemcpy.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check and swap in failed when rtMemcpy.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemcpy, dst, destMax, src, cnt, kind);
}

RUNTIME_HOOK_DEFINE(aclrtMemcpyImpl, void *dst, size_t destMax, const void *src, size_t count, aclrtMemcpyKind kind)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpyImpl, dst, destMax, src, count, kind);
    }
    LOG_DEBUG("Hook mem aclrtMemcpyImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check and swap in failed when aclrtMemcpyImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpyImpl, dst, destMax, src, count, kind);
}

RUNTIME_HOOK_DEFINE(rtsPointerGetAttributes, const void *ptr, rtPtrAttributes_t *attributes)
{
    LOG_DEBUG("Hook mem rtsPointerGetAttributes.");
    if (get_shm_state() == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, rtsPointerGetAttributes, ptr, attributes);
    }

    swap_hook_t *swap_hook = swap_hook_get_global();
    int ret = swap_hook_check_and_swap_in(swap_hook);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtsPointerGetAttributes.");

    return RUNTIME_HOOK_CALL(rt_library_entry, rtsPointerGetAttributes, ptr, attributes);
}

RUNTIME_HOOK_DEFINE(rtPointerGetAttributes, rtPointerAttributes_t *attributes, const void *ptr)
{
    LOG_DEBUG("Hook mem rtPointerGetAttributes.");

    if (get_shm_state() == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, rtPointerGetAttributes, attributes, ptr);
    }

    swap_hook_t *swap_hook = swap_hook_get_global();
    int ret = swap_hook_check_and_swap_in(swap_hook);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtPointerGetAttributes.");

    return RUNTIME_HOOK_CALL(rt_library_entry, rtPointerGetAttributes, attributes, ptr);
}

RUNTIME_HOOK_DEFINE(aclrtPointerGetAttributesImpl, const void *ptr, aclrtPtrAttributes *attributes)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtPointerGetAttributesImpl, ptr, attributes);
    }
    LOG_DEBUG("Hook mem aclrtPointerGetAttributesImpl.");

    if (get_shm_state() == NULL) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtPointerGetAttributesImpl, ptr, attributes);
    }

    swap_hook_t *swap_hook = swap_hook_get_global();
    int ret = swap_hook_check_and_swap_in(swap_hook);
    CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtPointerGetAttributesImpl.");

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtPointerGetAttributesImpl, ptr, attributes);
}

RUNTIME_HOOK_DEFINE(rtMemcpyAsync, void *dst, uint64_t destMax, const void *src, uint64_t cnt, rtMemcpyKind_t kind,
                    rtStream_t stm)
{
    LOG_DEBUG("Hook mem rtMemcpyAsync.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtMemcpyAsync.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemcpyAsync, dst, destMax, src, cnt, kind, stm);
}

RUNTIME_HOOK_DEFINE(aclrtMemcpyAsyncImpl, void *dst, size_t destMax, const void *src, size_t count,
                    aclrtMemcpyKind kind, aclrtStream stream)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpyAsyncImpl, dst, destMax, src, count, kind, stream);
    }
    LOG_DEBUG("Hook mem aclrtMemcpyAsyncImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemcpyAsyncImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpyAsyncImpl, dst, destMax, src, count, kind, stream);
}

RUNTIME_HOOK_DEFINE(rtsCheckMemType, void **addrs, uint32_t size, uint32_t memType, uint32_t *checkResult,
                    uint32_t reserve)
{
    LOG_DEBUG("Hook mem rtsCheckMemType.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtsCheckMemType.");
        if ((memType & RT_MEM_MASK_DEV_TYPE) != 0U && swap_hook != NULL && swap_hook->tracker != NULL &&
            addrs != NULL) {
            for (uint32_t i = 0; i < size; i++) {
                if (addrs[i] != NULL && memory_tracker_find_containing(swap_hook->tracker, addrs[i]) != NULL) {
                    memType |= RT_MEM_MASK_RSVD_TYPE;
                    break;
                }
            }
        }
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtsCheckMemType, addrs, size, memType, checkResult, reserve);
}

RUNTIME_HOOK_DEFINE(aclrtCheckMemTypeImpl, void **addrs, uint32_t size, uint32_t memType, uint32_t *checkResult,
                    uint32_t reserve)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtCheckMemTypeImpl, addrs, size, memType, checkResult, reserve);
    }
    LOG_DEBUG("Hook mem aclrtCheckMemTypeImpl.");

    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtCheckMemTypeImpl.");
        if ((memType & RT_MEM_MASK_DEV_TYPE) != 0U && swap_hook != NULL && swap_hook->tracker != NULL &&
            addrs != NULL) {
            for (uint32_t i = 0; i < size; i++) {
                if (addrs[i] != NULL && memory_tracker_find_containing(swap_hook->tracker, addrs[i]) != NULL) {
                    memType |= RT_MEM_MASK_RSVD_TYPE;
                    break;
                }
            }
        }
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtCheckMemTypeImpl, addrs, size, memType, checkResult, reserve);
}

RUNTIME_HOOK_DEFINE(rtMemcpyAsyncEx, void *dst, uint64_t destMax, const void *src, uint64_t cnt, rtMemcpyKind_t kind,
                    rtStream_t stm, rtMemcpyConfig_t *memcpyConfig)
{
    LOG_DEBUG("Hook mem rtMemcpyAsyncEx.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtMemcpyAsyncEx.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemcpyAsyncEx, dst, destMax, src, cnt, kind, stm, memcpyConfig);
}

RUNTIME_HOOK_DEFINE(aclrtMemcpyAsyncWithConditionImpl, void *dst, size_t destMax, const void *src, size_t count,
                    aclrtMemcpyKind kind, aclrtStream stream)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpyAsyncWithConditionImpl, dst, destMax, src, count, kind,
                                 stream);
    }
    LOG_DEBUG("Hook mem aclrtMemcpyAsyncWithConditionImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemcpyAsyncWithConditionImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpyAsyncWithConditionImpl, dst, destMax, src, count, kind,
                             stream);
}

RUNTIME_HOOK_DEFINE(rtsMemcpyBatch, void **dsts, void **srcs, size_t *sizes, size_t count, rtMemcpyBatchAttr *attrs,
                    size_t *attrsIdxs, size_t numAttrs, size_t *failIdx)
{
    LOG_DEBUG("Hook mem rtsMemcpyBatch.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtsMemcpyBatch.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtsMemcpyBatch, dsts, srcs, sizes, count, attrs, attrsIdxs, numAttrs,
                             failIdx);
}

RUNTIME_HOOK_DEFINE(aclrtMemcpyBatchImpl, void **dsts, size_t *destMaxs, void **srcs, size_t *sizes, size_t numBatches,
                    aclrtMemcpyBatchAttr *attrs, size_t *attrsIndexes, size_t numAttrs, size_t *failIndex)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpyBatchImpl, dsts, destMaxs, srcs, sizes, numBatches, attrs,
                                 attrsIndexes, numAttrs, failIndex);
    }
    LOG_DEBUG("Hook mem aclrtMemcpyBatchImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemcpyBatchImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpyBatchImpl, dsts, destMaxs, srcs, sizes, numBatches, attrs,
                             attrsIndexes, numAttrs, failIndex);
}

RUNTIME_HOOK_DEFINE(aclrtMemcpyBatchV2Impl, void **dsts, size_t *destMaxs, void **srcs, size_t *sizes,
                    size_t numBatches, aclrtMemcpyBatchAttr *attrs, size_t *attrsIndexes, size_t numAttrs)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpyBatchV2Impl, dsts, destMaxs, srcs, sizes, numBatches,
                                 attrs, attrsIndexes, numAttrs);
    }
    LOG_DEBUG("Hook mem aclrtMemcpyBatchV2Impl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemcpyBatchV2Impl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpyBatchV2Impl, dsts, destMaxs, srcs, sizes, numBatches, attrs,
                             attrsIndexes, numAttrs);
}

RUNTIME_HOOK_DEFINE(rtsMemcpyBatchAsync, void **dsts, size_t *destMaxs, void **srcs, size_t *sizes, size_t count,
                    rtMemcpyBatchAttr *attrs, size_t *attrsIdxs, size_t numAttrs, size_t *failIdx, rtStream_t stream)
{
    LOG_DEBUG("Hook mem rtsMemcpyBatchAsync.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtsMemcpyBatchAsync.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtsMemcpyBatchAsync, dsts, destMaxs, srcs, sizes, count, attrs,
                             attrsIdxs, numAttrs, failIdx, stream);
}

RUNTIME_HOOK_DEFINE(aclrtMemcpyBatchAsyncImpl, void **dsts, size_t *destMaxs, void **srcs, size_t *sizes,
                    size_t numBatches, aclrtMemcpyBatchAttr *attrs, size_t *attrsIndexes, size_t numAttrs,
                    size_t *failIndex, aclrtStream stream)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpyBatchAsyncImpl, dsts, destMaxs, srcs, sizes, numBatches,
                                 attrs, attrsIndexes, numAttrs, failIndex, stream);
    }
    LOG_DEBUG("Hook mem aclrtMemcpyBatchAsyncImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemcpyBatchAsyncImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpyBatchAsyncImpl, dsts, destMaxs, srcs, sizes, numBatches,
                             attrs, attrsIndexes, numAttrs, failIndex, stream);
}

RUNTIME_HOOK_DEFINE(aclrtMemcpyBatchAsyncV2Impl, void **dsts, size_t *destMaxs, void **srcs, size_t *sizes,
                    size_t numBatches, aclrtMemcpyBatchAttr *attrs, size_t *attrsIndexes, size_t numAttrs,
                    aclrtStream stream)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpyBatchAsyncV2Impl, dsts, destMaxs, srcs, sizes, numBatches,
                                 attrs, attrsIndexes, numAttrs, stream);
    }
    LOG_DEBUG("Hook mem aclrtMemcpyBatchAsyncV2Impl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemcpyBatchAsyncV2Impl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpyBatchAsyncV2Impl, dsts, destMaxs, srcs, sizes, numBatches,
                             attrs, attrsIndexes, numAttrs, stream);
}

RUNTIME_HOOK_DEFINE(rtMemcpy2d, void *dst, uint64_t dstPitch, const void *src, uint64_t srcPitch, uint64_t width,
                    uint64_t height, rtMemcpyKind_t kind)
{
    LOG_DEBUG("Hook mem rtMemcpy2d.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtMemcpy2d.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemcpy2d, dst, dstPitch, src, srcPitch, width, height, kind);
}

RUNTIME_HOOK_DEFINE(aclrtMemcpy2dImpl, void *dst, size_t dpitch, const void *src, size_t spitch, size_t width,
                    size_t height, aclrtMemcpyKind kind)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpy2dImpl, dst, dpitch, src, spitch, width, height, kind);
    }
    LOG_DEBUG("Hook mem aclrtMemcpy2dImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemcpy2dImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpy2dImpl, dst, dpitch, src, spitch, width, height, kind);
}

RUNTIME_HOOK_DEFINE(rtMemcpy2dAsync, void *dst, uint64_t dstPitch, const void *src, uint64_t srcPitch, uint64_t width,
                    uint64_t height, rtMemcpyKind_t kind, rtStream_t stm)
{
    LOG_DEBUG("Hook mem rtMemcpy2dAsync.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtMemcpy2dAsync.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemcpy2dAsync, dst, dstPitch, src, srcPitch, width, height, kind, stm);
}

RUNTIME_HOOK_DEFINE(aclrtMemcpy2dAsyncImpl, void *dst, size_t dpitch, const void *src, size_t spitch, size_t width,
                    size_t height, aclrtMemcpyKind kind, aclrtStream stream)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpy2dAsyncImpl, dst, dpitch, src, spitch, width, height,
                                 kind, stream);
    }
    LOG_DEBUG("Hook mem aclrtMemcpy2dAsyncImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemcpy2dAsyncImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpy2dAsyncImpl, dst, dpitch, src, spitch, width, height, kind,
                             stream);
}

RUNTIME_HOOK_DEFINE(rtsSetMemcpyDesc, rtMemcpyDesc_t desc, rtMemcpyKind kind, void *srcAddr, void *dstAddr,
                    size_t count, rtMemcpyConfig_t *config)
{
    LOG_DEBUG("Hook mem rtsSetMemcpyDesc.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtsSetMemcpyDesc.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtsSetMemcpyDesc, desc, kind, srcAddr, dstAddr, count, config);
}

RUNTIME_HOOK_DEFINE(aclrtSetMemcpyDescImpl, void *desc, aclrtMemcpyKind kind, void *srcAddr, void *dstAddr,
                    size_t count, void *config)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtSetMemcpyDescImpl, desc, kind, srcAddr, dstAddr, count, config);
    }
    LOG_DEBUG("Hook mem aclrtSetMemcpyDescImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtSetMemcpyDescImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtSetMemcpyDescImpl, desc, kind, srcAddr, dstAddr, count, config);
}

RUNTIME_HOOK_DEFINE(rtsMemcpyAsyncWithDesc, rtMemcpyDesc_t desc, rtMemcpyKind kind, rtMemcpyConfig_t *config,
                    rtStream_t stream)
{
    LOG_DEBUG("Hook mem rtsMemcpyAsyncWithDesc.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtsMemcpyAsyncWithDesc.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtsMemcpyAsyncWithDesc, desc, kind, config, stream);
}

RUNTIME_HOOK_DEFINE(aclrtMemcpyAsyncWithDescImpl, void *desc, aclrtMemcpyKind kind, aclrtStream stream)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpyAsyncWithDescImpl, desc, kind, stream);
    }
    LOG_DEBUG("Hook mem aclrtMemcpyAsyncWithDescImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemcpyAsyncWithDescImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpyAsyncWithDescImpl, desc, kind, stream);
}

RUNTIME_HOOK_DEFINE(rtMemcpyAsyncWithOffset, void **dst, uint64_t dstMax, uint64_t dstDataOffset, const void **src,
                    uint64_t cnt, uint64_t srcDataOffset, rtMemcpyKind kind, rtStream_t stm)
{
    LOG_DEBUG("Hook mem rtMemcpyAsyncWithOffset.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtMemcpyAsyncWithOffset.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemcpyAsyncWithOffset, dst, dstMax, dstDataOffset, src, cnt,
                             srcDataOffset, kind, stm);
}

RUNTIME_HOOK_DEFINE(aclrtMemcpyAsyncWithOffsetImpl, void **dst, size_t destMax, size_t dstDataOffset, const void **src,
                    size_t count, size_t srcDataOffset, aclrtMemcpyKind kind, aclrtStream stream)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpyAsyncWithOffsetImpl, dst, destMax, dstDataOffset, src,
                                 count, srcDataOffset, kind, stream);
    }
    LOG_DEBUG("Hook mem aclrtMemcpyAsyncWithOffsetImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemcpyAsyncWithOffsetImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemcpyAsyncWithOffsetImpl, dst, destMax, dstDataOffset, src, count,
                             srcDataOffset, kind, stream);
}

RUNTIME_HOOK_DEFINE(rtMemset, void *devPtr, uint64_t destMax, uint32_t val, uint64_t cnt)
{
    LOG_DEBUG("Hook mem rtMemset.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtMemset.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemset, devPtr, destMax, val, cnt);
}

RUNTIME_HOOK_DEFINE(aclrtMemsetImpl, void *devPtr, size_t maxCount, int32_t value, size_t count)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemsetImpl, devPtr, maxCount, value, count);
    }
    LOG_DEBUG("Hook mem aclrtMemsetImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemsetImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemsetImpl, devPtr, maxCount, value, count);
}

RUNTIME_HOOK_DEFINE(rtMemsetAsync, void *ptr, uint64_t destMax, uint32_t val, uint64_t cnt, rtStream_t stm)
{
    LOG_DEBUG("Hook mem rtMemsetAsync.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtMemsetAsync.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemsetAsync, ptr, destMax, val, cnt, stm);
}

RUNTIME_HOOK_DEFINE(aclrtMemsetAsyncImpl, void *devPtr, size_t maxCount, int32_t value, size_t count,
                    aclrtStream stream)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemsetAsyncImpl, devPtr, maxCount, value, count, stream);
    }
    LOG_DEBUG("Hook mem aclrtMemsetAsyncImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemsetAsyncImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemsetAsyncImpl, devPtr, maxCount, value, count, stream);
}

RUNTIME_HOOK_DEFINE(rtMemsetD32, void *dst, uint64_t destMax, uint32_t value, uint64_t count)
{
    LOG_DEBUG("Hook mem rtMemsetD32.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtMemsetD32.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemsetD32, dst, destMax, value, count);
}

RUNTIME_HOOK_DEFINE(aclrtMemsetD32Impl, void *ptr, size_t memSize, uint32_t value, size_t N)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemsetD32Impl, ptr, memSize, value, N);
    }
    LOG_DEBUG("Hook mem aclrtMemsetD32Impl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemsetD32Impl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemsetD32Impl, ptr, memSize, value, N);
}

RUNTIME_HOOK_DEFINE(rtMemsetD32Async, void *dst, uint64_t destMax, uint32_t value, uint64_t count, rtStream_t stm)
{
    LOG_DEBUG("Hook mem rtMemsetD32Async.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtMemsetD32Async.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemsetD32Async, dst, destMax, value, count, stm);
}

RUNTIME_HOOK_DEFINE(aclrtMemsetD32AsyncImpl, void *ptr, size_t memSize, uint32_t value, size_t N, aclrtStream stream)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemsetD32AsyncImpl, ptr, memSize, value, N, stream);
    }
    LOG_DEBUG("Hook mem aclrtMemsetD32AsyncImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemsetD32AsyncImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemsetD32AsyncImpl, ptr, memSize, value, N, stream);
}

RUNTIME_HOOK_DEFINE(rtMemPrefetchToDevice, void *devPtr, uint64_t len, int32_t devId)
{
    LOG_DEBUG("Hook mem rtMemPrefetchToDevice.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtMemPrefetchToDevice.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemPrefetchToDevice, devPtr, len, devId);
}

RUNTIME_HOOK_DEFINE(aclrtMemP2PMapImpl, void *devPtr, size_t size, int32_t dstDevId, uint64_t flags)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemP2PMapImpl, devPtr, size, dstDevId, flags);
    }
    LOG_DEBUG("Hook mem aclrtMemP2PMapImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemP2PMapImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemP2PMapImpl, devPtr, size, dstDevId, flags);
}

RUNTIME_HOOK_DEFINE(rtsIpcMemGetExportKey, const void *ptr, size_t size, char_t *key, uint32_t len, uint64_t flags)
{
    LOG_DEBUG("Hook mem rtsIpcMemGetExportKey.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtsIpcMemGetExportKey.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtsIpcMemGetExportKey, ptr, size, key, len, flags);
}

RUNTIME_HOOK_DEFINE(aclrtIpcMemGetExportKeyImpl, void *devPtr, size_t size, char *key, size_t len, uint64_t flags)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtIpcMemGetExportKeyImpl, devPtr, size, key, len, flags);
    }
    LOG_DEBUG("Hook mem aclrtIpcMemGetExportKeyImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtIpcMemGetExportKeyImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtIpcMemGetExportKeyImpl, devPtr, size, key, len, flags);
}

RUNTIME_HOOK_DEFINE(rtsIpcMemImportByKey, void **ptr, const char_t *key, uint64_t flags)
{
    LOG_DEBUG("Hook mem rtsIpcMemImportByKey.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtsIpcMemImportByKey.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtsIpcMemImportByKey, ptr, key, flags);
}

RUNTIME_HOOK_DEFINE(aclrtIpcMemImportByKeyImpl, void **devPtr, const char *key, uint64_t flags)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtIpcMemImportByKeyImpl, devPtr, key, flags);
    }
    LOG_DEBUG("Hook mem aclrtIpcMemImportByKeyImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtIpcMemImportByKeyImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtIpcMemImportByKeyImpl, devPtr, key, flags);
}

RUNTIME_HOOK_DEFINE(rtsValueWrite, const void *const devAddr, const uint64_t value, const uint32_t flag, rtStream_t stm)
{
    LOG_DEBUG("Hook mem rtsValueWrite.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtsValueWrite.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtsValueWrite, devAddr, value, flag, stm);
}

RUNTIME_HOOK_DEFINE(aclrtValueWriteImpl, void *devAddr, uint64_t value, uint32_t flag, aclrtStream stream)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtValueWriteImpl, devAddr, value, flag, stream);
    }
    LOG_DEBUG("Hook mem aclrtValueWriteImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtValueWriteImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtValueWriteImpl, devAddr, value, flag, stream);
}

RUNTIME_HOOK_DEFINE(rtsValueWait, const void *const devAddr, const uint64_t value, const uint32_t flag, rtStream_t stm)
{
    LOG_DEBUG("Hook mem rtsValueWait.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtsValueWait.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtsValueWait, devAddr, value, flag, stm);
}

RUNTIME_HOOK_DEFINE(aclrtValueWaitImpl, void *devAddr, uint64_t value, uint32_t flag, aclrtStream stream)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtValueWaitImpl, devAddr, value, flag, stream);
    }
    LOG_DEBUG("Hook mem aclrtValueWaitImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtValueWaitImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtValueWaitImpl, devAddr, value, flag, stream);
}

RUNTIME_HOOK_DEFINE(rtMemSetAccess, void *virPtr, size_t size, rtMemAccessDesc *desc, size_t count)
{
    LOG_DEBUG("Hook mem rtMemSetAccess.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtMemSetAccess.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemSetAccess, virPtr, size, desc, count);
}

RUNTIME_HOOK_DEFINE(aclrtMemSetAccessImpl, void *virPtr, size_t size, aclrtMemAccessDesc *desc, size_t count)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemSetAccessImpl, virPtr, size, desc, count);
    }
    LOG_DEBUG("Hook mem aclrtMemSetAccessImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemSetAccessImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemSetAccessImpl, virPtr, size, desc, count);
}

RUNTIME_HOOK_DEFINE(rtMemGetAccess, void *virPtr, rtMemLocation *location, uint64_t *flags)
{
    LOG_DEBUG("Hook mem rtMemGetAccess.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtMemGetAccess.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemGetAccess, virPtr, location, flags);
}

RUNTIME_HOOK_DEFINE(aclrtMemGetAccessImpl, void *virPtr, aclrtMemLocation *location, uint64_t *flag)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemGetAccessImpl, virPtr, location, flag);
    }
    LOG_DEBUG("Hook mem aclrtMemGetAccessImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemGetAccessImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemGetAccessImpl, virPtr, location, flag);
}

RUNTIME_HOOK_DEFINE(rtMemRetainAllocationHandle, void *virPtr, rtDrvMemHandle *handle)
{
    LOG_DEBUG("Hook mem rtMemRetainAllocationHandle.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtMemRetainAllocationHandle.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemRetainAllocationHandle, virPtr, handle);
}

RUNTIME_HOOK_DEFINE(aclrtMemRetainAllocationHandleImpl, void *virPtr, aclrtDrvMemHandle *handle)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemRetainAllocationHandleImpl, virPtr, handle);
    }
    LOG_DEBUG("Hook mem aclrtMemRetainAllocationHandleImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemRetainAllocationHandleImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemRetainAllocationHandleImpl, virPtr, handle);
}

RUNTIME_HOOK_DEFINE(rtMemGetAddressRange, void *ptr, void **pbase, size_t *psize)
{
    LOG_DEBUG("Hook mem rtMemGetAddressRange.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtMemGetAddressRange.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemGetAddressRange, ptr, pbase, psize);
}

RUNTIME_HOOK_DEFINE(aclrtMemGetAddressRangeImpl, void *ptr, void **pbase, size_t *psize)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemGetAddressRangeImpl, ptr, pbase, psize);
    }
    LOG_DEBUG("Hook mem aclrtMemGetAddressRangeImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemGetAddressRangeImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemGetAddressRangeImpl, ptr, pbase, psize);
}

RUNTIME_HOOK_DEFINE(rtMemMapSelectedLink, void *virPtrDst, size_t size, void *virPtrSrc, uint32_t linkIdx)
{
    LOG_DEBUG("Hook mem rtMemMapSelectedLink.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtMemMapSelectedLink.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemMapSelectedLink, virPtrDst, size, virPtrSrc, linkIdx);
}

RUNTIME_HOOK_DEFINE(aclrtMemMapSelectedLinkImpl, void *virPtrDst, size_t size, void *virPtrSrc, uint32_t linkIdx)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemMapSelectedLinkImpl, virPtrDst, size, virPtrSrc, linkIdx);
    }
    LOG_DEBUG("Hook mem aclrtMemMapSelectedLinkImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtMemMapSelectedLinkImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemMapSelectedLinkImpl, virPtrDst, size, virPtrSrc, linkIdx);
}

RUNTIME_HOOK_DEFINE(rtCmoAsync, void *srcAddrPtr, size_t srcLen, rtCmoOpCode_t cmoType, rtStream_t stm)
{
    LOG_DEBUG("Hook mem rtCmoAsync.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtCmoAsync.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtCmoAsync, srcAddrPtr, srcLen, cmoType, stm);
}

RUNTIME_HOOK_DEFINE(rtsCmoAsync, void *srcAddrPtr, size_t srcLen, rtCmoOpCode cmoType, rtStream_t stm)
{
    LOG_DEBUG("Hook mem rtsCmoAsync.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtsCmoAsync.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtsCmoAsync, srcAddrPtr, srcLen, cmoType, stm);
}

RUNTIME_HOOK_DEFINE(aclrtCmoAsyncImpl, void *src, size_t size, aclrtCmoType cmoType, aclrtStream stream)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtCmoAsyncImpl, src, size, cmoType, stream);
    }
    LOG_DEBUG("Hook mem aclrtCmoAsyncImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtCmoAsyncImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtCmoAsyncImpl, src, size, cmoType, stream);
}

RUNTIME_HOOK_DEFINE(rtsCmoAsyncWithBarrier, void *srcAddrPtr, size_t srcLen, rtCmoOpCode cmoType, uint32_t logicId,
                    rtStream_t stm)
{
    LOG_DEBUG("Hook mem rtsCmoAsyncWithBarrier.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtsCmoAsyncWithBarrier.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtsCmoAsyncWithBarrier, srcAddrPtr, srcLen, cmoType, logicId, stm);
}

RUNTIME_HOOK_DEFINE(aclrtCmoAsyncWithBarrierImpl, void *src, size_t size, aclrtCmoType cmoType, uint32_t barrierId,
                    aclrtStream stream)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtCmoAsyncWithBarrierImpl, src, size, cmoType, barrierId, stream);
    }
    LOG_DEBUG("Hook mem aclrtCmoAsyncWithBarrierImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtCmoAsyncWithBarrierImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtCmoAsyncWithBarrierImpl, src, size, cmoType, barrierId, stream);
}

RUNTIME_HOOK_DEFINE(rtsLaunchCmoAddrTask, rtCmoDesc_t cmoDesc, rtStream_t stm, rtCmoOpCode cmoOpCode,
                    const void *reserve)
{
    LOG_DEBUG("Hook mem rtsLaunchCmoAddrTask.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when rtsLaunchCmoAddrTask.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, rtsLaunchCmoAddrTask, cmoDesc, stm, cmoOpCode, reserve);
}

RUNTIME_HOOK_DEFINE(aclrtCmoAsyncWithDescImpl, void *cmoDesc, aclrtCmoType cmoType, aclrtStream stream,
                    const void *reserve)
{
    if (!get_aclrt_impl_hook_enable()) {
        return RUNTIME_HOOK_CALL(rt_library_entry, aclrtCmoAsyncWithDescImpl, cmoDesc, cmoType, stream, reserve);
    }
    LOG_DEBUG("Hook mem aclrtCmoAsyncWithDescImpl.");
    if (get_shm_state() != NULL) {
        swap_hook_t *swap_hook = swap_hook_get_global();
        int ret = swap_hook_check_and_swap_in(swap_hook);
        CHECK_RETURN_ERROR_CODE(ret, "Swap hook check failed when aclrtCmoAsyncWithDescImpl.");
    }

    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtCmoAsyncWithDescImpl, cmoDesc, cmoType, stream, reserve);
}