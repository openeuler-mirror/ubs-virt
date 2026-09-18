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

#include "log.h"
#include "mem_limiter.h"
#include "npu_manager.h"
#include "runtime_hook.h"

void load_rt_libraries(void);

RUNTIME_HOOK_DEFINE(rtMalloc, void **devPtr, uint64_t size, rtMemType_t type, const uint16_t moduleId)
{
    LOG_DEBUG("Hook mem rtMalloc size:%" PRIu64 ".", size);
    int ret = guard_memory(size);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtMalloc, devPtr, size, type, moduleId);
}

RUNTIME_HOOK_DEFINE(aclrtMallocImpl, void **devPtr, size_t size, aclrtMemMallocPolicy policy)
{
    LOG_DEBUG("Hook mem aclrtMallocImpl size:%" PRIu64 ".", size);
    int ret = guard_memory(size);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMallocImpl, devPtr, size, policy);
}

RUNTIME_HOOK_DEFINE(aclrtMallocAlign32Impl, void **devPtr, size_t size, aclrtMemMallocPolicy policy)
{
    LOG_DEBUG("Hook mem aclrtMallocAlign32Impl size:%" PRIu64 ".", size);
    int ret = guard_memory(size);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMallocAlign32Impl, devPtr, size, policy);
}

RUNTIME_HOOK_DEFINE(rtMallocCached, void **devPtr, uint64_t size, rtMemType_t type, const uint16_t moduleId)
{
    LOG_DEBUG("Hook mem rtMallocCached size:%" PRIu64 ".", size);
    int ret = guard_memory(size);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtMallocCached, devPtr, size, type, moduleId);
}

RUNTIME_HOOK_DEFINE(aclrtMallocCachedImpl, void **devPtr, size_t size, aclrtMemMallocPolicy policy)
{
    LOG_DEBUG("Hook mem aclrtMallocCachedImpl size:%" PRIu64 ".", size);
    int ret = guard_memory(size);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMallocCachedImpl, devPtr, size, policy);
}

RUNTIME_HOOK_DEFINE(rtDvppMalloc, void **devPtr, uint64_t size, uint16_t moduleId)
{
    LOG_DEBUG("Hook mem rtDvppMalloc size:%" PRIu64 ".", size);
    int ret = guard_memory(size);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtDvppMalloc, devPtr, size, moduleId);
}

RUNTIME_HOOK_DEFINE(rtDvppMallocWithFlag, void **devPtr, uint64_t size, uint32_t flag, const uint16_t moduleId)
{
    LOG_DEBUG("Hook mem rtDvppMallocWithFlag size:%" PRIu64 ".", size);
    int ret = guard_memory(size);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtDvppMallocWithFlag, devPtr, size, flag, moduleId);
}

RUNTIME_HOOK_DEFINE(rtMemAlloc, void **devPtr, uint64_t size, rtMallocPolicy policy, rtMallocAdvise advise,
                    rtMallocConfig_t *cfg)
{
    LOG_DEBUG("Hook mem rtMemAlloc size:%" PRIu64 ".", size);
    int ret = guard_memory(size);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemAlloc, devPtr, size, policy, advise, cfg);
}

RUNTIME_HOOK_DEFINE(rtMemAllocManaged, void **ptr, uint64_t size, uint32_t flag, const uint16_t moduleId)
{
    LOG_DEBUG("Hook mem rtMemAllocManaged size:%" PRIu64 ".", size);
    int ret = guard_memory(size);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtMemAllocManaged, ptr, size, flag, moduleId);
}

RUNTIME_HOOK_DEFINE(aclrtMemAllocManagedImpl, void **ptr, uint64_t size, uint32_t flag)
{
    LOG_DEBUG("Hook mem aclrtMemAllocManagedImpl size:%" PRIu64 ".", size);
    int ret = guard_memory(size);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMemAllocManagedImpl, ptr, size, flag);
}

RUNTIME_HOOK_DEFINE(rtMallocPhysical, rtDrvMemHandle *handle, size_t size, rtDrvMemProp_t *prop, uint64_t flags)
{
    LOG_DEBUG("Hook mem rtMallocPhysical size:%zd.", size);
    int ret = guard_memory(size);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, rtMallocPhysical, handle, size, prop, flags);
}

RUNTIME_HOOK_DEFINE(aclrtMallocPhysicalImpl, aclrtDrvMemHandle *handle, size_t size, const aclrtPhysicalMemProp *prop,
                    uint64_t flags)
{
    LOG_DEBUG("Hook mem aclrtMallocPhysicalImpl size:%zd.", size);
    int ret = guard_memory(size);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }
    return RUNTIME_HOOK_CALL(rt_library_entry, aclrtMallocPhysicalImpl, handle, size, prop, flags);
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
    size_t remain = quota - used;
    *freeSize = remain;
    *totalSize = quota;
    return RT_ERROR_NONE;
}

RUNTIME_HOOK_DEFINE(aclrtGetMemInfoImpl, aclrtMemAttr attr, size_t *free, size_t *total)
{
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
    size_t remain = quota - used;
    *free = remain;
    *total = quota;
    return ACL_RT_SUCCESS;
}