/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 */

#ifndef __SWAP_BUFFER_SHM_H__
#define __SWAP_BUFFER_SHM_H__

#include <stdint.h>
#include "common.h"

/*
 * mmap 同一个全局 POSIX shm 文件 /shm_swap_buffer（由 enpu-manager 创建, 
 * 所有物理 NPU 共用一个内存池）, 获取本进程有效的虚拟地址. 
 * 映射大小由 fstat 动态获取
 */
#define SHM_SWAP_BUFFER_NAME "/shm_swap_buffer"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * 映射 /shm_swap_buffer 到本进程地址空间. 
 * 返回: 成功返回 mmap 映射基址, 失败返回 NULL. 
 */
void *swap_buffer_shm_attach(void);

/*
 * 解除 swap_buffer_shm_attach 返回的映射. 
 * 仅 munmap, 不 shm_unlink（enpu-manager 负责销毁）. 
 * 返回: ENPU_SUCCESS / ENPU_FAIL. 
 */
int swap_buffer_shm_detach(void *base);

#if defined(__cplusplus)
}
#endif

#endif
