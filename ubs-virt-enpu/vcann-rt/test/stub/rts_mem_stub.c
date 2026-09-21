/*
 * Mock VMM API stub implementation
 */

#include <string.h>
#include "rts_mem.h"

static bool g_vmm_fail_mode = false;
static int g_vmm_call_count = 0;
static int g_vmm_last_error = RT_ERROR_NONE;

void vmm_mock_set_fail(bool fail)
{
    g_vmm_fail_mode = fail;
}

void vmm_mock_reset(void)
{
    g_vmm_fail_mode = false;
    g_vmm_call_count = 0;
    g_vmm_last_error = RT_ERROR_NONE;
}

int vmm_mock_get_call_count(void)
{
    return g_vmm_call_count;
}

int vmm_mock_get_last_error(void)
{
    return g_vmm_last_error;
}

int rtsMemUnmap(void *ptr)
{
    g_vmm_call_count++;
    if (g_vmm_fail_mode) {
        g_vmm_last_error = RT_ERROR_INVALID_VALUE;
        return RT_ERROR_INVALID_VALUE;
    }
    (void)ptr;
    return RT_ERROR_NONE;
}

int rtsMemcpy(void *dst, void *src, uint64_t size, int direction)
{
    g_vmm_call_count++;
    if (g_vmm_fail_mode) {
        g_vmm_last_error = RT_ERROR_INVALID_VALUE;
        return RT_ERROR_INVALID_VALUE;
    }
    if (direction == 1) {
        memcpy(dst, src, size);
    } else if (direction == 2) {
        memcpy(dst, src, size);
    }
    return RT_ERROR_NONE;
}

int rtsMemFreePhysical(rtDrvMemHandle *handle)
{
    g_vmm_call_count++;
    if (g_vmm_fail_mode) {
        g_vmm_last_error = RT_ERROR_INVALID_VALUE;
        return RT_ERROR_INVALID_VALUE;
    }
    *handle = NULL;
    return RT_ERROR_NONE;
}

int rtsMemMallocPhysical(rtDrvMemHandle *handle, size_t size, rtDrvMemProp_t *prop, uint64_t flags)
{
    g_vmm_call_count++;
    if (g_vmm_fail_mode) {
        g_vmm_last_error = RT_ERROR_MEMORY_ALLOC_FAILED;
        return RT_ERROR_MEMORY_ALLOC_FAILED;
    }
    *handle = (void *)1;
    if (prop != NULL) {
        prop->side = (uint32_t)size;
    }
    (void)flags;
    return RT_ERROR_NONE;
}

int rtsMemMap(void *ptr, size_t size, rtDrvMemHandle handle)
{
    g_vmm_call_count++;
    if (g_vmm_fail_mode) {
        g_vmm_last_error = RT_ERROR_INVALID_VALUE;
        return RT_ERROR_INVALID_VALUE;
    }
    (void)ptr;
    (void)size;
    (void)handle;
    return RT_ERROR_NONE;
}