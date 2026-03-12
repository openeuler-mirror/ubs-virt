/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 *
 * ubs-optimizer is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef COLLECTOR
#define COLLECTOR

#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <cstdint>

#include <sys/resource.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <linux/perf_event.h>

using  u32 = uint32_t;

enum class CollectorStatus {
    SUCCESS = 0,
    COLLECTION_RUNNING,
    OPEN_BPF_PROGRAM_FAILED,
    LOAD_BPF_FAILED,
    CREAT_RINGBUF_FAILED,
};

struct ring_buffer_deleter {
    void operator () (struct ring_buffer *rb) const
    {
        if (rb) {
            ring_buffer__free(rb);
        }
    }
};

using WarpedRingbufObj = std::unique_ptr<struct ring_buffer, ring_buffer_deleter>;

class Collector {
public:
    virtual CollectorStatus launchCollecting() = 0;

    [[nodiscard]] bool checkRunning() const;
    void collectionTask();
    virtual void stopCollecting();

protected:
    std::atomic<bool> stopFlag{ false };
    bool collecting_{ false };
    std::unique_ptr<std::thread> collectionThread_;
    WarpedRingbufObj ringbuf;
    std::mutex mutex_;
};

#endif
