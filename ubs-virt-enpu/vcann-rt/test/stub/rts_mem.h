/*
 * Mock VMM API for SwapExecutor tests
 */

#ifndef __RTS_MEM_H__
#define __RTS_MEM_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "mem_base.h"

#ifndef RT_ERROR_NONE
#define RT_ERROR_NONE 0
#endif
#ifndef RT_ERROR_INVALID_VALUE
#define RT_ERROR_INVALID_VALUE 1
#endif
#ifndef RT_ERROR_MEMORY_ALLOC_FAILED
#define RT_ERROR_MEMORY_ALLOC_FAILED 2
#endif

int rtsMemUnmap(void *ptr);
int rtsMemcpy(void *dst, void *src, uint64_t size, int direction);
int rtsMemFreePhysical(rtDrvMemHandle *handle);
int rtsMemMallocPhysical(rtDrvMemHandle *handle, size_t size, rtDrvMemProp_t *prop, uint64_t flags);
int rtsMemMap(void *ptr, size_t size, rtDrvMemHandle handle);

void vmm_mock_set_fail(bool fail);
void vmm_mock_reset(void);
int vmm_mock_get_call_count(void);

#endif
