/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 */

#ifndef __SWAP_BUFFER_SHM_H__
#define __SWAP_BUFFER_SHM_H__

#include <stddef.h>
#include "common.h"

#define SHM_SWAP_BUFFER_NAME "/shm_swap_buffer"

typedef struct {
    void *base;
    size_t size;
} swap_buffer_mapping_t;

#if defined(__cplusplus)
extern "C" {
#endif

/* 挂载成功后填充映射；调用者传入空映射。 */
int swap_buffer_shm_attach_by_name(const char *shm_name, swap_buffer_mapping_t *mapping);
/* 供命令监控线程使用：仅尝试一次，不进行重试休眠。 */
int swap_buffer_shm_try_attach_by_name(const char *shm_name, swap_buffer_mapping_t *mapping);
/* 解除本地映射；共享内存段由 enpu-manager 管理。 */
int swap_buffer_shm_detach(swap_buffer_mapping_t *mapping);

#if defined(__cplusplus)
}
#endif

#endif
