/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 */

#include "enpu_manager.h"
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../internal/npu_allocator.h"
#include "../internal/npu_allocator_internal.h"
#include "allocation.h"
#include "common.h"
#include "config_manager.h"
#include "dcmi_wrapper.h"
#include "enpu_config.h"
#include "log.h"
#include "pod_watchdog.h"
#include "securec.h"
#include "shm_manager.h"
#include "swap_buffer_manager.h"
#include "swap_resolver.h"
#include "swap_resolver_thread.h"

#define SWAP_RESOLVER_POLL_INTERVAL_MS 10

struct enpu_manager {
    npu_allocator_t *allocator;
    swap_resolver_t *swap_resolver;
    swap_resolver_thread_t *swap_resolver_threads[MAX_NPU_PER_NODE];
    pod_watchdog_t *watchdog;
    pthread_t watchdog_thread;
    pthread_mutex_t reconfig_lock;
    atomic_bool running;
    atomic_bool watchdog_running;
    uint64_t watchdog_poll_interval_ms;
};

#define CHECK_NULL_RET(param, ret) \
    do {                           \
        if ((param) == NULL) {     \
            return (ret);          \
        }                          \
    } while (0)

typedef struct {
    swap_status_response_t *resp;
    const device_info_t *devices;
    int device_count;
    int error;
} swapped_vnpu_collector_ctx_t;

static void collect_swapped_vnpu_for_status(int phy_id, int vnpu_id, uint64_t swap_size, void *user_data)
{
    swapped_vnpu_collector_ctx_t *ctx = (swapped_vnpu_collector_ctx_t *)user_data;
    if (ctx->resp->swapped_count >= MAX_VNPU_PER_DIE * MAX_NPU_PER_NODE) {
        return;
    }

    const char *die_id = NULL;
    for (int i = 0; i < ctx->device_count; i++) {
        if (ctx->devices[i].phy_id == phy_id) {
            die_id = ctx->devices[i].meta.die_id;
            break;
        }
    }
    if (die_id == NULL || die_id[0] == '\0') {
        LOG_ERROR("[ENPU-MGR] Failed to find DIE UUID for swapped vNPU: phy_id=%d, vnpu_id=%d", phy_id, vnpu_id);
        ctx->error = ENPU_NOT_FOUND;
        return;
    }

    swapped_model_info_t *info = &ctx->resp->swapped_models[ctx->resp->swapped_count];
    int ret = snprintf_s(info->die_id, sizeof(info->die_id), sizeof(info->die_id) - 1, "%s", die_id);
    if (ret < 0) {
        LOG_ERROR("[ENPU-MGR] Failed to copy DIE UUID for swapped vNPU: phy_id=%d, vnpu_id=%d", phy_id, vnpu_id);
        ctx->error = ENPU_FAIL;
        return;
    }

    info->vnpu_id = vnpu_id;
    info->swap_size = swap_size;
    ctx->resp->swapped_count++;
}

static void *watchdog_thread_func(void *arg)
{
    enpu_manager_t *mgr = (enpu_manager_t *)arg;

    LOG_INFO("[ENPU-MGR] Watchdog thread started");

    while (atomic_load(&mgr->watchdog_running)) {
        pod_watchdog_poll_and_cleanup(mgr->watchdog);
        struct timespec ts = {
            .tv_sec = (time_t)(mgr->watchdog_poll_interval_ms / 1000),
            .tv_nsec = (long)((mgr->watchdog_poll_interval_ms % 1000) * 1000000ULL),
        };
        nanosleep(&ts, NULL);
    }

    LOG_INFO("[ENPU-MGR] Watchdog thread stopped");
    return NULL;
}

static int find_pod_allocation(enpu_manager_t *mgr, const char *pod_uid, int *phy_id, int *vnpu_id);
static int enpu_manager_swap_clean_impl(enpu_manager_t *mgr, int phy_id, int vnpu_id);
static int enpu_manager_swap_force_out_impl(enpu_manager_t *mgr, int phy_id, int vnpu_id, const char *pod_uid);
static int enpu_manager_swap_force_in_impl(enpu_manager_t *mgr, int phy_id, int vnpu_id, const char *pod_uid);

int enpu_manager_destroy(enpu_manager_t *mgr);

static void enpu_manager_copy_str(char *dst, size_t dst_len, const char *src)
{
    if (dst == NULL || dst_len == 0 || src == NULL) {
        return;
    }
    int ret = strncpy_s(dst, dst_len, src, strnlen(src, dst_len - 1));
    if (ret != 0) {
        LOG_ERROR("[ENPU-MGR] strncpy_s failed, ret=%d", ret);
    }
}

static int enpu_manager_init_allocator(enpu_manager_t *mgr, const enpu_manager_config_t *config)
{
    allocator_config_t alloc_config = {0};
    if (config != NULL) {
        enpu_manager_copy_str(alloc_config.config_base_path, sizeof(alloc_config.config_base_path),
                              config->config_base_path);
        enpu_manager_copy_str(alloc_config.checkpoint_path, sizeof(alloc_config.checkpoint_path),
                              config->checkpoint_path);
        enpu_manager_copy_str(alloc_config.evaluator_name, sizeof(alloc_config.evaluator_name), config->evaluator_name);
        enpu_manager_copy_str(alloc_config.share_strategy, sizeof(alloc_config.share_strategy), config->share_strategy);
        alloc_config.use_dcmi_stub = config->use_dcmi_stub;
        alloc_config.dcmi_stub = config->dcmi_stub;
    } else {
        enpu_manager_copy_str(alloc_config.config_base_path, sizeof(alloc_config.config_base_path), CONFIG_BASE_PATH);
        enpu_manager_copy_str(alloc_config.checkpoint_path, sizeof(alloc_config.checkpoint_path),
                              DEFAULT_CHECKPOINT_PATH);
        enpu_manager_copy_str(alloc_config.evaluator_name, sizeof(alloc_config.evaluator_name), "share");
        alloc_config.use_dcmi_stub = false;
    }

    mgr->allocator = npu_allocator_create(&alloc_config);
    if (mgr->allocator == NULL) {
        LOG_ERROR("[ENPU-MGR] Failed to create allocator");
        return ENPU_FAIL;
    }

    int ret = npu_allocator_init(mgr->allocator);
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("[ENPU-MGR] Failed to init allocator: %d", ret);
        npu_allocator_destroy(mgr->allocator);
        mgr->allocator = NULL;
        return ret;
    }

    return ENPU_SUCCESS;
}

static void destroy_all_swap_pools(npu_allocator_t *allocator);

static bool swap_pool_size(uint64_t hbm_mb, double ratio, uint64_t *size)
{
    if (!(isfinite(ratio) && (ratio) >= 0.0))
        return false;
    long double bytes = ratio > 0.0 ? (1.0L + ratio) * hbm_mb * 1024.0L * 1024.0L : 0.0L;
    if (bytes > INT64_MAX || bytes > SIZE_MAX)
        return false;
    *size = (uint64_t)bytes;
    return true;
}

static bool read_host_mem_free_bytes(uint64_t *free_bytes);

static int recover_unused_swap_pools(npu_allocator_t *allocator, const bool phy_present[MAX_NPU_PER_NODE])
{
    for (int phy_id = 0; phy_id < MAX_NPU_PER_NODE; phy_id++) {
        if (!phy_present[phy_id])
            continue;
        bool found = false;
        int result = shm_swap_buffer_has_pool_files(phy_id, &found);
        if (result != ENPU_SUCCESS)
            return result;
        if (!found)
            continue;
        char die_id[DIE_ID_LEN];
        if (npu_allocator_get_die_id(allocator, phy_id, die_id, sizeof(die_id)) != ENPU_SUCCESS)
            return ENPU_FAIL;
        shm_manager_t *shm_mgr = npu_allocator_get_shm_manager(allocator);
        if (shm_manager_get_or_create_shm(shm_mgr, phy_id, die_id, NULL, 0) != ENPU_SUCCESS)
            return ENPU_FAIL;
        shm_state_t *state = shm_manager_get_state(shm_mgr, phy_id);
        bool idle = false;
        char reason[128] = {0};
        result = npu_allocator_reconfig_begin(allocator, phy_id, &idle, reason, sizeof(reason));
        if (result == ENPU_SUCCESS) {
            if (idle) {
                result = shm_swap_buffer_recover_orphans(state, phy_id, true);
            } else {
                /* 卡上仍有存活 vNPU：残留池可能保存着它们的换出数据，必须保留待复用 */
                LOG_INFO("[ENPU-MGR] phy=%d still in use (%s), keep residual swap pools for reuse", phy_id, reason);
            }
            npu_allocator_reconfig_end(allocator);
        }
        if (result != ENPU_SUCCESS) {
            LOG_ERROR("[ENPU-MGR] Cannot reclaim swap pools for phy=%d, ret=%d, reason=%s", phy_id, result, reason);
            return result;
        }
    }
    return ENPU_SUCCESS;
}

enpu_manager_t *enpu_manager_create(const enpu_manager_config_t *config)
{
    enpu_manager_t *mgr = (enpu_manager_t *)calloc(1, sizeof(enpu_manager_t));
    if (mgr == NULL) {
        LOG_ERROR("[ENPU-MGR] Failed to allocate manager memory");
        return NULL;
    }

    if (enpu_manager_init_allocator(mgr, config) != ENPU_SUCCESS) {
        free(mgr);
        return NULL;
    }

    const double MB_TO_BYTES = (double)(1024ULL * 1024ULL);
    uint64_t die_hbm_mb[MAX_NPU_PER_NODE] = {0};
    if (npu_allocator_get_per_die_hbm_mb(mgr->allocator, die_hbm_mb, MAX_NPU_PER_NODE) != ENPU_SUCCESS) {
        LOG_WARN("[ENPU-MGR] Warning: failed to get per-die HBM, swap may be under-sized");
    }

    /* 配置单值会广播到数组容量上限。用真实拓扑区分“不存在的设备”和
     * “存在但 HBM 未知的设备”，不能把 HBM=0 本身作为设备不存在的判据。 */
    device_info_t devices[MAX_NPU_PER_NODE];
    int device_count = MAX_NPU_PER_NODE;
    bool phy_present[MAX_NPU_PER_NODE] = {false};
    if (enpu_manager_query_devices(mgr, NULL, devices, &device_count) != ENPU_SUCCESS || device_count <= 0 ||
        device_count > MAX_NPU_PER_NODE) {
        LOG_ERROR("[ENPU-MGR] Failed to query physical topology for oversub configuration");
        npu_allocator_destroy(mgr->allocator);
        free(mgr);
        return NULL;
    }
    for (int i = 0; i < device_count; i++) {
        int phy_id = devices[i].phy_id;
        if (phy_id < 0 || phy_id >= MAX_NPU_PER_NODE) {
            LOG_ERROR("[ENPU-MGR] Invalid physical device id %d", phy_id);
            npu_allocator_destroy(mgr->allocator);
            free(mgr);
            return NULL;
        }
        phy_present[phy_id] = true;
    }

    /* 根据每个die的超分比例 累计节点级 swap_buf_size */
    bool any_swap_enabled = false;
    bool config_invalid = false;
    uint64_t swap_buf_size = 0;
    for (int i = 0; i < MAX_NPU_PER_NODE; i++) {
        double ratio = (config != NULL) ? config->oversub_ratio[i] : 0.0;
        /* 与 enpu_manager_set_oversub_ratio 同一套域校验（NaN/负数） */
        if (!(isfinite(ratio) && (ratio) >= 0.0)) {
            LOG_ERROR("[ENPU-MGR] invalid oversub_ratio %.2f for phy=%d (must be a non-negative finite "
                      "number), aborting",
                      ratio, i);
            config_invalid = true;
            break;
        }
        if (!phy_present[i]) {
            if (ratio > 0.0) {
                LOG_WARN("[ENPU-MGR] ignoring oversub_ratio=%.2f for absent phy=%d", ratio, i);
            }
            mgr->allocator->oversub_ratio[i] = 0.0;
            continue;
        }
        mgr->allocator->oversub_ratio[i] = ratio;
        if (ratio > 0.0) {
            /* ratio>0 但 HBM 未知就无法确定池大小：硬失败，避免留下"ratio>0 但无池"的 die */
            if (die_hbm_mb[i] == 0) {
                LOG_ERROR("[ENPU-MGR] phy=%d oversub-ratio=%.2f but HBM size unknown, aborting", i, ratio);
                config_invalid = true;
                break;
            }
            uint64_t per_size = 0;
            if (!swap_pool_size(die_hbm_mb[i], ratio, &per_size) || per_size > UINT64_MAX - swap_buf_size) {
                config_invalid = true;
                break;
            }
            swap_buf_size += per_size;
            any_swap_enabled = true;
        }
    }
    if (config_invalid) {
        npu_allocator_destroy(mgr->allocator);
        free(mgr);
        return NULL;
    }
    LOG_INFO("[ENPU-MGR] swap_enabled_at_startup=%d, swap_buffer_size=%lu bytes", any_swap_enabled ? 1 : 0,
             (unsigned long)swap_buf_size);
    for (int i = 0; i < MAX_NPU_PER_NODE; i++) {
        if (die_hbm_mb[i] > 0) {
            LOG_INFO("[ENPU-MGR]   phy=%d  die_hbm=%lu MB  oversub_ratio=%.2f  -> %s", i, (unsigned long)die_hbm_mb[i],
                     mgr->allocator->oversub_ratio[i],
                     (mgr->allocator->oversub_ratio[i] > 0.0) ? "swap-on" : "swap-off");
        }
    }

    /* 在内存检查和建池前回收无使用者的残留段，覆盖 SIGKILL/OOM 后的启动。 */
    if (recover_unused_swap_pools(mgr->allocator, phy_present) != ENPU_SUCCESS) {
        npu_allocator_destroy(mgr->allocator);
        free(mgr);
        return NULL;
    }

    /* 启动时检查全节点换出池容量是否低于宿主机 MemFree。 */
    if (any_swap_enabled) {
        uint64_t mem_free;
        if (!read_host_mem_free_bytes(&mem_free)) {
            LOG_ERROR("[ENPU-MGR] failed to read MemFree from /proc/meminfo");
            npu_allocator_destroy(mgr->allocator);
            free(mgr);
            return NULL;
        }
        if (swap_buf_size >= mem_free) {
            LOG_ERROR("[ENPU-MGR] configured oversub-ratio too large: total swap buffer %lu bytes >= host MemFree "
                      "%lu bytes, aborting",
                      (unsigned long)swap_buf_size, (unsigned long)mem_free);
            npu_allocator_destroy(mgr->allocator);
            free(mgr);
            return NULL;
        }
    }

    /* swap_resolver / watchdog 是节点级常驻组件，与"启动时有没有 die 开 swap"无关：
     * 全 0 启动后仍可在线开启（见 enpu_manager_set_oversub_ratio），故无条件创建。
     * 两者都只 calloc 结构、不起线程，线程在 enpu_manager_start() 里拉起。 */
    shm_manager_t *shm_mgr = npu_allocator_get_shm_manager(mgr->allocator);

    int pre_wm = (config != NULL && config->swap_pre_watermark > 0) ? config->swap_pre_watermark : 80;
    mgr->swap_resolver = swap_resolver_create(mgr->allocator, NULL, pre_wm / 100.0, 30ULL * 1000000000ULL);
    if (mgr->swap_resolver == NULL) {
        LOG_ERROR("[ENPU-MGR] Failed to create swap resolver");
        npu_allocator_destroy(mgr->allocator);
        mgr->allocator = NULL;
        free(mgr);
        return NULL;
    }

    mgr->watchdog = pod_watchdog_create(shm_mgr, NULL, mgr->allocator);
    if (mgr->watchdog == NULL) {
        LOG_ERROR("[ENPU-MGR] Failed to create watchdog");
        swap_resolver_destroy(mgr->swap_resolver);
        mgr->swap_resolver = NULL;
        npu_allocator_destroy(mgr->allocator);
        mgr->allocator = NULL;
        free(mgr);
        return NULL;
    }

    /* 启动时就带 swap 的 die：逐 die 建池并起 resolver 线程。
     * 运行期首次开启由 set_oversub_ratio 准备新池和线程。 */
    if (any_swap_enabled) {
        /* per-die 建池：先 get_or_create 该 die 的 shm_state（写 swap_pool_name），再建池。
         * 建池失败必须硬失败——否则会留下"ratio>0 但无池"的 die，其换出永远失败。 */
        bool pools_ok = true;
        for (int phy_id = 0; phy_id < MAX_NPU_PER_NODE; phy_id++) {
            if (mgr->allocator->oversub_ratio[phy_id] <= 0.0 || die_hbm_mb[phy_id] <= 0) {
                continue;
            }
            char die_id[DIE_ID_LEN] = {0};
            if (npu_allocator_get_die_id(mgr->allocator, phy_id, die_id, DIE_ID_LEN) != ENPU_SUCCESS) {
                LOG_ERROR("[ENPU-MGR] phy_id=%d oversub enabled but no die_id", phy_id);
                pools_ok = false;
                break;
            }
            char shm_id_out[SHM_ID_LEN] = {0};
            if (shm_manager_get_or_create_shm(shm_mgr, phy_id, die_id, shm_id_out, SHM_ID_LEN) != ENPU_SUCCESS) {
                LOG_ERROR("[ENPU-MGR] get_or_create shm failed for phy_id=%d", phy_id);
                pools_ok = false;
                break;
            }
            shm_state_t *state = npu_allocator_get_shm_state(mgr->allocator, phy_id);
            if (state == NULL) {
                LOG_ERROR("[ENPU-MGR] shm_state NULL for phy_id=%d", phy_id);
                pools_ok = false;
                break;
            }
            /* hbm_total 用于 watermark 计算 usage_ratio，此前只有分配路径的
             * shm_manager_init_state() 会写；建池时该 die 上还没有容器，若不在此补齐，
             * resolver 每 10ms 就会打一条 "hbm_total=0"。恢复路径写入的非 0 值不覆盖。 */
            if (state->hbm_total == 0) {
                shm_manager_init_state(shm_mgr, phy_id, (uint64_t)((double)die_hbm_mb[phy_id] * MB_TO_BYTES), true);
            }
            uint64_t per_size = 0;
            swap_pool_size(die_hbm_mb[phy_id], mgr->allocator->oversub_ratio[phy_id], &per_size);
            if (shm_swap_buffer_pool_init(state, phy_id, per_size) != ENPU_SUCCESS) {
                LOG_ERROR("[ENPU-MGR] Failed to create swap buffer pool for phy_id=%d", phy_id);
                pools_ok = false;
                break;
            }
        }
        if (!pools_ok) {
            destroy_all_swap_pools(mgr->allocator);
            /* 线程尚未启动，直接销毁 */
            pod_watchdog_destroy(mgr->watchdog);
            mgr->watchdog = NULL;
            swap_resolver_destroy(mgr->swap_resolver);
            mgr->swap_resolver = NULL;
            npu_allocator_destroy(mgr->allocator);
            mgr->allocator = NULL;
            free(mgr);
            return NULL;
        }

        /* 池已就绪、resolver/watchdog 线程未启动：按残留换出记录重建 TLSF 台账，
         * 防止重启后把仍被容器占用的区间重复分配出去。 */
        if (shm_swap_buffer_recover(npu_allocator_get_shm_manager(mgr->allocator)) != ENPU_SUCCESS) {
            LOG_WARN("[ENPU-MGR] swap ledger rebuild lost residual regions; they may be overwritten by later "
                     "swap-out, check swap buffer status");
        }

        /* per-die 起 swap_resolver_thread，按 phy_id 索引存放 */
        for (int phy_id = 0; phy_id < MAX_NPU_PER_NODE; phy_id++) {
            if (mgr->allocator->oversub_ratio[phy_id] <= 0.0 || die_hbm_mb[phy_id] <= 0) {
                continue;
            }
            swap_resolver_thread_t *srt =
                swap_resolver_thread_create(mgr->swap_resolver, phy_id, SWAP_RESOLVER_POLL_INTERVAL_MS);
            if (srt == NULL) {
                LOG_WARN("[ENPU-MGR] Failed to create swap_resolver_thread for phy_id=%d, swap on this die disabled",
                         phy_id);
                continue;
            }
            mgr->swap_resolver_threads[phy_id] = srt;
        }
    }

    mgr->watchdog_poll_interval_ms =
        (config != NULL && config->watchdog_poll_interval_ms > 0) ? config->watchdog_poll_interval_ms : 500;

    atomic_store(&mgr->running, false);
    atomic_store(&mgr->watchdog_running, false);

    pthread_mutex_init(&mgr->reconfig_lock, NULL);
    LOG_INFO("[ENPU-MGR] Manager created and initialized");
    return mgr;
}

int enpu_manager_destroy(enpu_manager_t *mgr)
{
    CHECK_NULL_RET(mgr, ENPU_INVALID_PARAM);

    /* 与 set_oversub_ratio 串行，避免销毁过程中池/线程被并发重建 */
    pthread_mutex_lock(&mgr->reconfig_lock);

    /* 未先调用 stop() 时也要先停线程，否则 destroy 后线程仍访问已释放对象 */
    if (mgr->watchdog != NULL) {
        if (atomic_load(&mgr->watchdog_running)) {
            atomic_store(&mgr->watchdog_running, false);
            pthread_join(mgr->watchdog_thread, NULL);
        }
        pod_watchdog_destroy(mgr->watchdog);
        mgr->watchdog = NULL;
    }

    for (int i = 0; i < MAX_NPU_PER_NODE; i++) {
        if (mgr->swap_resolver_threads[i] != NULL) {
            /* stop 幂等（started 标志），重复 stop 安全 */
            swap_resolver_thread_stop(mgr->swap_resolver_threads[i]);
            swap_resolver_thread_destroy(mgr->swap_resolver_threads[i]);
            mgr->swap_resolver_threads[i] = NULL;
        }
    }

    if (mgr->swap_resolver != NULL) {
        swap_resolver_destroy(mgr->swap_resolver);
        mgr->swap_resolver = NULL;
    }

    /* 逐 die 销毁 swap buffer 池（在销毁 allocator/shm_mgr 之前） */
    destroy_all_swap_pools(mgr->allocator);

    if (mgr->allocator != NULL) {
        npu_allocator_destroy(mgr->allocator);
        mgr->allocator = NULL;
    }

    pthread_mutex_unlock(&mgr->reconfig_lock);
    pthread_mutex_destroy(&mgr->reconfig_lock);
    free(mgr);

    LOG_INFO("[ENPU-MGR] Manager destroyed");
    return ENPU_SUCCESS;
}

static int enpu_manager_start_impl(enpu_manager_t *mgr)
{
    CHECK_NULL_RET(mgr, ENPU_INVALID_PARAM);

    atomic_store(&mgr->running, true);

    if (mgr->watchdog != NULL) {
        atomic_store(&mgr->watchdog_running, true);
        int wd_ret = pthread_create(&mgr->watchdog_thread, NULL, watchdog_thread_func, mgr);
        if (wd_ret != 0) {
            LOG_ERROR("[ENPU-MGR] Failed to create watchdog thread: %d", wd_ret);
            atomic_store(&mgr->watchdog_running, false);
            atomic_store(&mgr->running, false);
            return ENPU_FAIL;
        }
    }

    // 每个phy_id对应一个swap_resolver_thread（swap 关闭时数组全空，循环为空）
    int started = 0;
    for (int i = 0; i < MAX_NPU_PER_NODE; i++) {
        if (mgr->swap_resolver_threads[i] == NULL) {
            continue;
        }
        int sret = swap_resolver_thread_start(mgr->swap_resolver_threads[i]);
        if (sret != ENPU_SUCCESS) {
            LOG_WARN("[ENPU-MGR] Failed to start swap_resolver_thread %d: %d", i, sret);
        } else {
            started++;
        }
    }

    LOG_INFO("[ENPU-MGR] Manager started with watchdog thread and %d swap resolver threads", started);
    return ENPU_SUCCESS;
}

static int enpu_manager_stop_impl(enpu_manager_t *mgr)
{
    CHECK_NULL_RET(mgr, ENPU_INVALID_PARAM);

    for (int i = 0; i < MAX_NPU_PER_NODE; i++) {
        if (mgr->swap_resolver_threads[i] != NULL) {
            swap_resolver_thread_stop(mgr->swap_resolver_threads[i]);
        }
    }

    atomic_store(&mgr->running, false);

    /* watchdog 现在是无条件创建的（节点级常驻），不能再拿 watchdog != NULL 当作
     * "线程已拉起"：create 后未 start()、或 start() 里 pthread_create 失败时
     * watchdog_thread 都是未初始化的，直接 join 是未定义行为。用 running 标志判定。 */
    if (mgr->watchdog != NULL && atomic_load(&mgr->watchdog_running)) {
        atomic_store(&mgr->watchdog_running, false);
        pthread_join(mgr->watchdog_thread, NULL);
    }

    LOG_INFO("[ENPU-MGR] Manager stopped");
    return ENPU_SUCCESS;
}

int enpu_manager_allocate(enpu_manager_t *mgr, alloc_request_t *req, alloc_response_t *resp)
{
    CHECK_NULL_RET(mgr, ENPU_INVALID_PARAM);
    CHECK_NULL_RET(req, ENPU_INVALID_PARAM);
    CHECK_NULL_RET(resp, ENPU_INVALID_PARAM);

    if (mgr->allocator == NULL) {
        LOG_ERROR("[ENPU-MGR] Allocator not initialized");
        if (snprintf_s(resp->error_msg, sizeof(resp->error_msg), sizeof(resp->error_msg) - 1,
                       "allocator not initialized") < 0) {
            LOG_ERROR("[ENPU-MGR] snprintf_s failed");
        }
        resp->result = ENPU_FAIL;
        return ENPU_FAIL;
    }

    return npu_allocator_allocate(mgr->allocator, req, resp);
}

int enpu_manager_release(enpu_manager_t *mgr, const char *pod_uid, const char *container_name)
{
    CHECK_NULL_RET(mgr, ENPU_INVALID_PARAM);
    CHECK_NULL_RET(pod_uid, ENPU_INVALID_PARAM);
    CHECK_NULL_RET(container_name, ENPU_INVALID_PARAM);

    if (mgr->allocator == NULL) {
        LOG_ERROR("[ENPU-MGR] Allocator not initialized");
        return ENPU_FAIL;
    }

    return npu_allocator_release(mgr->allocator, pod_uid, container_name);
}

int enpu_manager_release_by_id(enpu_manager_t *mgr, const char *allocation_id)
{
    CHECK_NULL_RET(mgr, ENPU_INVALID_PARAM);
    CHECK_NULL_RET(allocation_id, ENPU_INVALID_PARAM);

    if (mgr->allocator == NULL) {
        LOG_ERROR("[ENPU-MGR] Allocator not initialized");
        return ENPU_FAIL;
    }

    return npu_allocator_release_by_id(mgr->allocator, allocation_id);
}

int enpu_manager_release_all(enpu_manager_t *mgr, int *released_count)
{
    CHECK_NULL_RET(mgr, ENPU_INVALID_PARAM);

    if (mgr->allocator == NULL) {
        LOG_ERROR("[ENPU-MGR] Allocator not initialized");
        return ENPU_FAIL;
    }

    /* 列出当前所有 allocation，逐个 release */
    allocation_t allocs[MAX_ALLOC_COUNT];
    int count = 0;
    allocation_query_t query = {0};
    query.phy_id = -1;
    int ret = npu_allocator_query_allocations(mgr->allocator, &query, allocs, &count);
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("[ENPU-MGR] release_all: query failed: %d", ret);
        return ret;
    }

    int released = 0;
    for (int i = 0; i < count; i++) {
        int r = npu_allocator_release_by_id(mgr->allocator, allocs[i].allocation_id);
        if (r == ENPU_SUCCESS) {
            released++;
        } else {
            LOG_WARN("[ENPU-MGR] release_all: release allocation_id=%s failed: %d", allocs[i].allocation_id, r);
        }
    }

    if (released_count != NULL) {
        *released_count = released;
    }
    LOG_INFO("[ENPU-MGR] release_all: cleaned %d/%d allocations", released, count);
    return ENPU_SUCCESS;
}

int enpu_manager_query_devices(enpu_manager_t *mgr, const device_query_t *query, device_info_t *devices, int *count)
{
    CHECK_NULL_RET(mgr, ENPU_INVALID_PARAM);
    CHECK_NULL_RET(count, ENPU_INVALID_PARAM);

    if (mgr->allocator == NULL) {
        return ENPU_FAIL;
    }

    int phy_id_filter = -1;
    if (query != NULL) {
        phy_id_filter = query->phy_id;
    }

    npu_meta_t meta[MAX_NPU_PER_NODE];
    npu_allocatable_t allocatable[MAX_NPU_PER_NODE];
    int ret = npu_allocator_query_devices(mgr->allocator, phy_id_filter, meta, allocatable, count);
    if (ret != ENPU_SUCCESS) {
        return ret;
    }

    if (devices != NULL) {
        for (int i = 0; i < *count; i++) {
            if (memcpy_s(&devices[i].meta, sizeof(devices[i].meta), &meta[i], sizeof(npu_meta_t)) != 0) {
                LOG_ERROR("[ENPU-MGR] memcpy_s meta failed");
                return ENPU_FAIL;
            }
            if (memcpy_s(&devices[i].allocatable, sizeof(devices[i].allocatable), &allocatable[i],
                         sizeof(npu_allocatable_t)) != 0) {
                LOG_ERROR("[ENPU-MGR] memcpy_s allocatable failed");
                return ENPU_FAIL;
            }
            devices[i].phy_id = meta[i].phy_id;
            int32_t phy = meta[i].phy_id;
            devices[i].oversub_ratio =
                (phy >= 0 && phy < MAX_NPU_PER_NODE) ? npu_allocator_get_oversub_ratio(mgr->allocator, phy) : 0.0;
        }
    }

    return ENPU_SUCCESS;
}

int enpu_manager_query_allocations(enpu_manager_t *mgr, const allocation_query_t *query, allocation_t *allocations,
                                   int *count)
{
    CHECK_NULL_RET(mgr, ENPU_INVALID_PARAM);
    CHECK_NULL_RET(count, ENPU_INVALID_PARAM);

    if (mgr->allocator == NULL) {
        return ENPU_FAIL;
    }

    return npu_allocator_query_allocations(mgr->allocator, query, allocations, count);
}

char *enpu_manager_get_config_path(enpu_manager_t *mgr, const char *allocation_id, char *path, int path_size)
{
    if (mgr == NULL || mgr->allocator == NULL) {
        return NULL;
    }
    return config_manager_get_config_path(mgr->allocator->config_mgr, allocation_id, path, path_size);
}

int enpu_manager_set_swap_pre_watermark(enpu_manager_t *mgr, int percent)
{
    if (mgr == NULL || percent < 1 || percent > 100) {
        return ENPU_INVALID_PARAM;
    }
    /* A node with swap disabled has no resolver; the saved value applies if swap is enabled later. */
    if (mgr->swap_resolver == NULL) {
        return ENPU_SUCCESS;
    }
    return swap_resolver_set_pre_swap_watermark(mgr->swap_resolver, percent);
}

int enpu_manager_set_share_strategy(enpu_manager_t *mgr, const char *strategy)
{
    if (mgr == NULL || mgr->allocator == NULL) {
        return ENPU_FAIL;
    }
    return npu_allocator_set_share_strategy(mgr->allocator, strategy);
}

int enpu_manager_swap_control(enpu_manager_t *mgr, const char *pod_uid, swap_action_t action)
{
    CHECK_NULL_RET(mgr, ENPU_INVALID_PARAM);
    CHECK_NULL_RET(pod_uid, ENPU_INVALID_PARAM);

    int phy_id = 0, vnpu_id = 0;
    int ret = find_pod_allocation(mgr, pod_uid, &phy_id, &vnpu_id);
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("[ENPU-MGR] Pod not found for swap control: %s", pod_uid);
        return ENPU_NOT_FOUND;
    }

    switch (action) {
        case SWAP_ACTION_CLEAN:
            return enpu_manager_swap_clean_impl(mgr, phy_id, vnpu_id);
        case SWAP_ACTION_FORCE_OUT:
            return enpu_manager_swap_force_out_impl(mgr, phy_id, vnpu_id, pod_uid);
        case SWAP_ACTION_FORCE_IN:
            return enpu_manager_swap_force_in_impl(mgr, phy_id, vnpu_id, pod_uid);
        default:
            return ENPU_INVALID_PARAM;
    }
}

int enpu_manager_control(enpu_manager_t *mgr, control_op_t op)
{
    CHECK_NULL_RET(mgr, ENPU_INVALID_PARAM);

    if (mgr->allocator == NULL) {
        return ENPU_FAIL;
    }

    switch (op) {
        case CONTROL_OP_CHECKPOINT:
            return npu_allocator_checkpoint(mgr->allocator);
        case CONTROL_OP_RECOVER:
            return npu_allocator_recover(mgr->allocator);
        case CONTROL_OP_HEALTH_CHECK:
            return ENPU_SUCCESS;
        default:
            return ENPU_INVALID_PARAM;
    }
}

int enpu_manager_swap_status(enpu_manager_t *mgr, swap_status_response_t *resp)
{
    CHECK_NULL_RET(mgr, ENPU_INVALID_PARAM);
    CHECK_NULL_RET(resp, ENPU_INVALID_PARAM);

    memset_s(resp, sizeof(swap_status_response_t), 0, sizeof(swap_status_response_t));

    /* 用"当前是否有池"判定，不能用启动快照：全 0 启动后在线开启的节点也要能查状态 */
    if (!shm_swap_buffer_any_inited()) {
        LOG_ERROR("[ENPU-MGR] swap not enabled");
        return ENPU_FAIL;
    }

    int ret = shm_swap_buffer_get_status_aggregate(&resp->buffer_status);
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("[ENPU-MGR] Failed to get swap buffer status: %d", ret);
        return ret;
    }

    device_info_t devices[MAX_NPU_PER_NODE] = {0};
    int device_count = 0;
    ret = enpu_manager_query_devices(mgr, NULL, devices, &device_count);
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("[ENPU-MGR] Failed to query devices for swap status: %d", ret);
        return ret;
    }

    swapped_vnpu_collector_ctx_t ctx = {
        .resp = resp,
        .devices = devices,
        .device_count = device_count,
        .error = ENPU_SUCCESS,
    };
    ret = npu_allocator_foreach_swapped_vnpu(mgr->allocator, collect_swapped_vnpu_for_status, &ctx);
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("[ENPU-MGR] Failed to foreach swapped vnpu: %d", ret);
        return ret;
    }
    if (ctx.error != ENPU_SUCCESS) {
        return ctx.error;
    }

    return ENPU_SUCCESS;
}

static int find_pod_allocation(enpu_manager_t *mgr, const char *pod_uid, int *phy_id, int *vnpu_id)
{
    if (mgr->allocator == NULL) {
        return ENPU_NOT_FOUND;
    }

    allocation_registry_t *registry = npu_allocator_get_registry(mgr->allocator);
    if (registry == NULL) {
        return ENPU_NOT_FOUND;
    }

    allocation_t allocations[MAX_ALLOC_COUNT];
    int count = 0;
    int ret = allocation_registry_list(registry, allocations, &count);
    if (ret != ENPU_SUCCESS) {
        return ENPU_NOT_FOUND;
    }

    for (int i = 0; i < count; i++) {
        if (strcmp(allocations[i].pod_uid, pod_uid) == 0) {
            *vnpu_id = allocations[i].vnpu_id;
            *phy_id = allocations[i].phy_id;
            return ENPU_SUCCESS;
        }
    }

    return ENPU_NOT_FOUND;
}

static int enpu_manager_swap_clean_impl(enpu_manager_t *mgr, int phy_id, int vnpu_id)
{
    if (mgr->allocator == NULL || !shm_swap_buffer_any_inited()) {
        LOG_ERROR("[ENPU-MGR] swap managers not initialized");
        return ENPU_FAIL;
    }

    bool swapped = false;
    uint64_t offset = 0, size = 0;
    int ret = npu_allocator_get_swap_state(mgr->allocator, phy_id, vnpu_id, &swapped, &offset, &size);
    if (ret == ENPU_NOT_FOUND) {
        return ENPU_NOT_FOUND;
    }
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("[ENPU-MGR] Failed to get swap state: %d", ret);
        return ret;
    }

    if (!swapped) {
        return ENPU_SUCCESS;
    }

    ret = npu_allocator_clear_swap(mgr->allocator, phy_id, vnpu_id);
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("[ENPU-MGR] Failed to clear swap state: %d", ret);
        return ret;
    }

    return ENPU_SUCCESS;
}

static int enpu_manager_swap_force_out_impl(enpu_manager_t *mgr, int phy_id, int vnpu_id, const char *pod_uid)
{
    if (mgr->swap_resolver == NULL) {
        LOG_ERROR("[ENPU-MGR] swap_resolver not initialized");
        return ENPU_FAIL;
    }

    int ret =
        swap_resolver_write_swap_command(mgr->swap_resolver, phy_id, pod_uid, vnpu_id, SWAP_ACTION_OUT, SWAP_OUT_NONE);
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("[ENPU-MGR] Failed to write swap out command: %d", ret);
        return ret;
    }

    return ENPU_SUCCESS;
}

static int enpu_manager_swap_force_in_impl(enpu_manager_t *mgr, int phy_id, int vnpu_id, const char *pod_uid)
{
    if (mgr->allocator == NULL) {
        LOG_ERROR("[ENPU-MGR] allocator not initialized");
        return ENPU_FAIL;
    }

    int ret = npu_allocator_write_swap_cmd(mgr->allocator, phy_id, pod_uid, vnpu_id, SWAP_ACTION_IN, SWAP_OUT_NONE);
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("[ENPU-MGR] Failed to write swap in command: %d", ret);
        return ret;
    }

    return ENPU_SUCCESS;
}

static void destroy_all_swap_pools(npu_allocator_t *allocator)
{
    if (allocator == NULL) {
        return;
    }
    /* 仍有活跃 vNPU 时保留共享段：容器继续使用池中的换出数据，重启后按名复用 */
    bool keep_shared = npu_allocator_has_active_vnpu(allocator);
    for (int i = 0; i < MAX_NPU_PER_NODE; i++) {
        if (shm_swap_buffer_pool_is_inited(i)) {
            shm_swap_buffer_pool_deinit(npu_allocator_get_shm_state(allocator, i), i, keep_shared);
        }
    }
}

static bool read_host_mem_free_bytes(uint64_t *free_bytes)
{
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL)
        return false;

    unsigned long long mem_free_kb = 0;
    bool found_free = false;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf_s(line, "MemFree: %llu kB", &mem_free_kb) == 1) {
            found_free = true;
            break;
        }
    }
    bool read_ok = !ferror(fp);
    fclose(fp);
    if (!read_ok || !found_free || mem_free_kb > UINT64_MAX / 1024ULL)
        return false;
    *free_bytes = (uint64_t)mem_free_kb * 1024ULL;
    return true;
}

static int check_swap_pool_total(const uint64_t *die_hbm_mb, const double *ratios, int phy_id, uint64_t growth)
{
    uint64_t total = growth;
    for (int i = 0; i < MAX_NPU_PER_NODE; i++) {
        if (i == phy_id || die_hbm_mb[i] == 0)
            continue;
        uint64_t size;
        if (!swap_pool_size(die_hbm_mb[i], ratios[i], &size) || size > UINT64_MAX - total) {
            LOG_ERROR("[ENPU-MGR] cannot calculate total swap pool capacity for phy_id=%d while expanding phy_id=%d", i,
                      phy_id);
            return ENPU_FAIL;
        }
        total += size;
    }

    uint64_t mem_free;
    if (!read_host_mem_free_bytes(&mem_free)) {
        LOG_ERROR("[ENPU-MGR] failed to read MemFree from /proc/meminfo");
        return ENPU_FAIL;
    }
    if (total >= mem_free) {
        LOG_ERROR("[ENPU-MGR] total swap pool capacity %llu must be less than host MemFree %llu for phy_id=%d",
                  (unsigned long long)total, (unsigned long long)mem_free, phy_id);
        return ENPU_NO_RESOURCE;
    }
    return ENPU_SUCCESS;
}

static void stop_die_resolver_thread(enpu_manager_t *mgr, int phy_id)
{
    if (mgr->swap_resolver_threads[phy_id] == NULL) {
        return;
    }
    swap_resolver_thread_stop(mgr->swap_resolver_threads[phy_id]);
    swap_resolver_thread_destroy(mgr->swap_resolver_threads[phy_id]);
    mgr->swap_resolver_threads[phy_id] = NULL;
}

static int start_die_resolver_paused(enpu_manager_t *mgr, int phy_id)
{
    swap_resolver_thread_t *srt =
        swap_resolver_thread_create(mgr->swap_resolver, phy_id, SWAP_RESOLVER_POLL_INTERVAL_MS);
    if (srt == NULL) {
        LOG_ERROR("[ENPU-MGR] failed to recreate swap_resolver_thread for phy_id=%d", phy_id);
        return ENPU_FAIL;
    }
    int ret = swap_resolver_thread_pause(srt);
    if (ret != ENPU_SUCCESS) {
        swap_resolver_thread_destroy(srt);
        return ret;
    }
    ret = atomic_load(&mgr->running) ? swap_resolver_thread_start(srt) : ENPU_SUCCESS;
    if (ret != ENPU_SUCCESS) {
        LOG_ERROR("[ENPU-MGR] failed to start swap_resolver_thread for phy_id=%d: %d", phy_id, ret);
        swap_resolver_thread_resume(srt);
        swap_resolver_thread_destroy(srt);
        return ret;
    }
    mgr->swap_resolver_threads[phy_id] = srt;
    return ENPU_SUCCESS;
}

typedef struct oversub_ratio_save {
    enpu_manager_t *mgr;
    const char *path;
    double *ratios;
    int phy_id;
    int phy_id_span;
    double ratio;
    bool *failed;
} oversub_ratio_save_t;

static int save_oversub_ratio(void *context)
{
    oversub_ratio_save_t *save = context;
    if (save->path == NULL)
        return ENPU_SUCCESS;
    save->ratios[save->phy_id] = save->ratio;
    int result = enpu_config_write_oversub_ratio(save->path, save->ratios, save->phy_id_span);
    *save->failed = result != ENPU_SUCCESS;
    return result;
}

static int prepare_resolver_and_save(void *context)
{
    oversub_ratio_save_t *save = context;
    if (save->ratio > 0 && save->mgr->swap_resolver_threads[save->phy_id] == NULL) {
        /* 池已准备好，先确认线程可创建，再持久化配置。线程在发布后才开始轮询。 */
        int ret = start_die_resolver_paused(save->mgr, save->phy_id);
        if (ret != ENPU_SUCCESS)
            return ret;
    }
    return save_oversub_ratio(save);
}

static int set_oversub_ratio(enpu_manager_t *mgr, int phy_id, double ratio, const char *config_path, bool *save_failed)
{
    if (save_failed != NULL)
        *save_failed = false;
    CHECK_NULL_RET(mgr, ENPU_INVALID_PARAM);
    if (phy_id < 0 || phy_id >= MAX_NPU_PER_NODE) {
        return ENPU_INVALID_PARAM;
    }
    /* 比例必须是有限的非负数，负零也拒绝。 */
    if (!(isfinite(ratio) && (ratio) >= 0.0) || signbit(ratio)) {
        return ENPU_INVALID_PARAM;
    }
    if (mgr->allocator == NULL) {
        return ENPU_FAIL;
    }

    const double MB_TO_BYTES = (double)(1024ULL * 1024ULL);

    uint64_t die_hbm_mb[MAX_NPU_PER_NODE] = {0};
    npu_allocator_get_per_die_hbm_mb(mgr->allocator, die_hbm_mb, MAX_NPU_PER_NODE);

    if (die_hbm_mb[phy_id] == 0) {
        LOG_ERROR("[ENPU-MGR] set_oversub_ratio: phy_id=%d has no HBM info (unknown die)", phy_id);
        return ENPU_INVALID_PARAM;
    }

    uint64_t new_size = 0;
    if (!swap_pool_size(die_hbm_mb[phy_id], ratio, &new_size)) {
        return ENPU_INVALID_PARAM;
    }

    /* resolver 是节点级常驻组件（create 时无条件创建），这里只做防御：
     * 为 NULL 说明 create 中途失败，不该进入本函数 */
    if (new_size > 0 && mgr->swap_resolver == NULL) {
        LOG_ERROR("[ENPU-MGR] set_oversub_ratio: swap_resolver missing (manager not fully initialized?), "
                  "cannot enable swap on die %d",
                  phy_id);
        return ENPU_FAIL;
    }

    pthread_mutex_lock(&mgr->reconfig_lock);

    /* 串行校验扩容量并提交本次调整。 */
    double cur_ratio[MAX_NPU_PER_NODE] = {0};
    if (npu_allocator_get_oversub_ratios(mgr->allocator, cur_ratio) != ENPU_SUCCESS) {
        pthread_mutex_unlock(&mgr->reconfig_lock);
        return ENPU_FAIL;
    }
    int phy_id_span = 0;
    for (int i = 0; i < MAX_NPU_PER_NODE; i++) {
        if (die_hbm_mb[i] > 0)
            phy_id_span = i + 1;
    }
    oversub_ratio_save_t save = {
        .mgr = mgr,
        .path = config_path,
        .ratios = cur_ratio,
        .phy_id = phy_id,
        .phy_id_span = phy_id_span,
        .ratio = ratio,
        .failed = save_failed,
    };

    bool old_inited = shm_swap_buffer_pool_is_inited(phy_id);
    swap_buffer_status_t old_pool = {0};
    if (old_inited && shm_swap_buffer_pool_get_status(phy_id, &old_pool) != ENPU_SUCCESS) {
        pthread_mutex_unlock(&mgr->reconfig_lock);
        return ENPU_FAIL;
    }
    uint64_t old_size = old_pool.total;

    /* 幂等：ratio 未变且池状态与期望一致时直接返回。
     * 否则运行中重复下发同一 ratio 会因"有容器"被判 busy，且白白触发一次删池/建池+rt 重挂。 */
    bool pool_matches = (new_size == 0) ?
                            !old_inited :
                            (old_inited && old_size == new_size && mgr->swap_resolver_threads[phy_id] != NULL);
    if (ratio == cur_ratio[phy_id] && pool_matches) {
        bool ready = false;
        char reason[128] = {0};
        int result = npu_allocator_reconfig_begin(mgr->allocator, phy_id, &ready, reason, sizeof(reason));
        if (result == ENPU_SUCCESS) {
            npu_allocator_reconfig_end(mgr->allocator);
            result = ready ? save_oversub_ratio(&save) : ENPU_BUSY;
        }
        pthread_mutex_unlock(&mgr->reconfig_lock);
        if (result != ENPU_SUCCESS) {
            LOG_WARN("[ENPU-MGR] ratio update rejected for phy=%d, ret=%d, reason=%s", phy_id, result, reason);
            return result;
        }
        LOG_INFO("[ENPU-MGR] set_oversub_ratio: phy_id=%d ratio=%.2f unchanged, no-op", phy_id, ratio);
        return result;
    }

    /* 扩容时校验其他卡的池容量与目标卡的扩容增量之和。 */
    if (new_size > old_size) {
        int ret = check_swap_pool_total(die_hbm_mb, cur_ratio, phy_id, new_size - old_size);
        if (ret != ENPU_SUCCESS) {
            pthread_mutex_unlock(&mgr->reconfig_lock);
            return ret;
        }
    }

    /* 取该 die 的 shm_state：必须在拿 allocator 锁之前（getter 自身会加锁） */
    shm_state_t *state = npu_allocator_get_shm_state(mgr->allocator, phy_id);
    if (state == NULL) {
        char die_id[DIE_ID_LEN] = {0};
        if (npu_allocator_get_die_id(mgr->allocator, phy_id, die_id, DIE_ID_LEN) == ENPU_SUCCESS) {
            char shm_id_out[SHM_ID_LEN] = {0};
            shm_manager_get_or_create_shm(npu_allocator_get_shm_manager(mgr->allocator), phy_id, die_id, shm_id_out,
                                          SHM_ID_LEN);
            state = npu_allocator_get_shm_state(mgr->allocator, phy_id);
        }
    }
    if (new_size > 0 && state == NULL) {
        /* 无 shm_state 就无法写 swap_pool_name，也就无法通知 rt —— 绝不能留下"ratio>0 但无池"的 die */
        pthread_mutex_unlock(&mgr->reconfig_lock);
        LOG_ERROR("[ENPU-MGR] set_oversub_ratio: no shm_state for phy_id=%d, cannot build pool", phy_id);
        return ENPU_FAIL;
    }

    /* 在线首次启用时 state 是刚建的，hbm_total 仍为 0；同启动路径一并补齐 */
    if (state != NULL && state->hbm_total == 0 && die_hbm_mb[phy_id] > 0) {
        shm_manager_init_state(npu_allocator_get_shm_manager(mgr->allocator), phy_id,
                               (uint64_t)((double)die_hbm_mb[phy_id] * MB_TO_BYTES), true);
    }

    swap_resolver_thread_t *old_thread = mgr->swap_resolver_threads[phy_id];
    if (old_thread != NULL && swap_resolver_thread_pause(old_thread) != ENPU_SUCCESS) {
        pthread_mutex_unlock(&mgr->reconfig_lock);
        return ENPU_FAIL;
    }

    bool ready = false;
    char reason[128] = {0};
    int ret = npu_allocator_reconfig_begin(mgr->allocator, phy_id, &ready, reason, sizeof(reason));
    if (ret == ENPU_SUCCESS) {
        if (!ready) {
            ret = ENPU_BUSY;
        } else {
            ret = shm_swap_buffer_pool_update(state, phy_id, new_size, prepare_resolver_and_save, &save);
        }
        if (ret == ENPU_SUCCESS)
            mgr->allocator->oversub_ratio[phy_id] = ratio;
        npu_allocator_reconfig_end(mgr->allocator);
    }
    swap_resolver_thread_t *thread = mgr->swap_resolver_threads[phy_id];
    if (thread != NULL) {
        bool stop_thread = (ret != ENPU_SUCCESS && old_thread == NULL) || (ret == ENPU_SUCCESS && new_size == 0);
        /* 解锁前标记退出，避免线程再轮询一次。等待退出时 allocator 锁已释放。 */
        if (stop_thread)
            swap_resolver_thread_request_stop(thread);
        swap_resolver_thread_resume(thread);
        if (stop_thread)
            stop_die_resolver_thread(mgr, phy_id);
    }
    if (ret != ENPU_SUCCESS) {
        LOG_WARN("[ENPU-MGR] ratio update rejected for phy=%d, ret=%d, reason=%s", phy_id, ret, reason);
        pthread_mutex_unlock(&mgr->reconfig_lock);
        return ret;
    }

    pthread_mutex_unlock(&mgr->reconfig_lock);
    LOG_INFO("[ENPU-MGR] set_oversub_ratio: phy_id=%d ratio=%.2f done", phy_id, ratio);
    return ENPU_SUCCESS;
}

int enpu_manager_set_oversub_ratio(enpu_manager_t *mgr, int phy_id, double ratio)
{
    return set_oversub_ratio(mgr, phy_id, ratio, NULL, NULL);
}

int enpu_manager_set_oversub_ratio_and_save(enpu_manager_t *mgr, int phy_id, double ratio, const char *config_path,
                                            bool *save_failed)
{
    if (save_failed != NULL)
        *save_failed = false;
    if (config_path == NULL || save_failed == NULL)
        return ENPU_INVALID_PARAM;
    return set_oversub_ratio(mgr, phy_id, ratio, config_path, save_failed);
}

int enpu_manager_get_oversub_ratio(enpu_manager_t *mgr, double out_ratio[MAX_NPU_PER_NODE])
{
    CHECK_NULL_RET(mgr, ENPU_INVALID_PARAM);
    CHECK_NULL_RET(out_ratio, ENPU_INVALID_PARAM);
    return npu_allocator_get_oversub_ratios(mgr->allocator, out_ratio);
}

int enpu_manager_start(enpu_manager_t *mgr)
{
    CHECK_NULL_RET(mgr, ENPU_INVALID_PARAM);
    pthread_mutex_lock(&mgr->reconfig_lock);
    int ret = atomic_load(&mgr->running) ? ENPU_SUCCESS : enpu_manager_start_impl(mgr);
    pthread_mutex_unlock(&mgr->reconfig_lock);
    return ret;
}

int enpu_manager_stop(enpu_manager_t *mgr)
{
    CHECK_NULL_RET(mgr, ENPU_INVALID_PARAM);
    pthread_mutex_lock(&mgr->reconfig_lock);
    int ret = enpu_manager_stop_impl(mgr);
    pthread_mutex_unlock(&mgr->reconfig_lock);
    return ret;
}
