#ifndef __MEM_BASE_H__
#define __MEM_BASE_H__

#include <stdint.h>

typedef void *rtDrvMemHandle;
#ifndef RT_MEM_TYPES_STUB_DEFINED
#define RT_MEM_TYPES_STUB_DEFINED

typedef enum
{
    RT_MEMCPY_KIND_HOST_TO_HOST = 0,
    RT_MEMCPY_KIND_HOST_TO_DEVICE,
    RT_MEMCPY_KIND_DEVICE_TO_HOST,
    RT_MEMCPY_KIND_DEVICE_TO_DEVICE,
    RT_MEMCPY_KIND_DEFAULT,
    RT_MEMCPY_KIND_HOST_TO_BUF_TO_DEVICE,
    RT_MEMCPY_KIND_INNER_DEVICE_TO_DEVICE,
    RT_MEMCPY_KIND_INTER_DEVICE_TO_DEVICE,
    RT_MEMCPY_KIND_MAX,
} rtMemcpyKind;

typedef enum
{
    RT_MEMORY_LOC_HOST = 0,
    RT_MEMORY_LOC_DEVICE,
    RT_MEMORY_LOC_UNREGISTERED,
    RT_MEMORY_LOC_MANAGED,
    RT_MEMORY_LOC_HOST_NUMA,
    RT_MEMORY_LOC_MAX,
    RT_MEMORY_LOC_UVM_MANAGED,
} rtMemLocationType;

typedef struct {
    uint32_t id;
    rtMemLocationType type;
} rtMemLocation;

typedef struct {
    rtMemLocation location;
    uint32_t pageSize;
    uint32_t rsv[4];
} rtPtrAttributes_t;

typedef struct {
    rtMemLocation dstLoc;
    rtMemLocation srcLoc;
    uint8_t rsv[16];
} rtMemcpyBatchAttr;

typedef struct {
    void *dst;
    void *src;
    uint64_t dstPitch;
    uint64_t srcPitch;
    uint64_t width;
    uint64_t height;
    rtMemcpyKind kind;
} rtMemcpy2DParams_t;

typedef struct {
    void *dst;
    uint64_t destMax;
    const void *src;
    uint64_t count;
    rtMemcpyKind kind;
} rtMemcpyConfig_t;

typedef struct {
    void *dst;
    uint64_t destMax;
    const void *src;
    uint64_t count;
    rtMemcpyKind kind;
    uint32_t flag;
} rtMemcpyDesc_t;

#define ACL_MEM_LOCATION_TYPE_DEVICE 0
#endif

#ifndef RT_DRV_MEM_PROP_T_DEFINED
#define RT_DRV_MEM_PROP_T_DEFINED
typedef struct DrvMemProp {
    uint32_t side;
    uint32_t devid;
    uint32_t module_id;
    uint32_t pg_type;
    uint32_t mem_type;
    uint64_t reserve;
} rtDrvMemProp_t;
#endif

#endif /* __MEM_BASE_H__ */
