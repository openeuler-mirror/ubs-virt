/*
 * Mock shm stub for SwapMonitor test
 */

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define SWAP_ACTION_NONE 0
#define SWAP_ACTION_OUT 1
#define SWAP_ACTION_IN 2

typedef struct mock_shm_state {
    char target_pod_uid[64];
    int target_vnpu_id;
    int action;
    atomic_bool completed;
    atomic_bool swapped;
    uint64_t swap_offset;
    uint64_t swap_size;
    atomic_bool stop_flag;
} mock_shm_state_t;

static mock_shm_state_t g_mock_shm = {0};

void mock_shm_reset(void)
{
    memset(&g_mock_shm, 0, sizeof(g_mock_shm));
    atomic_store(&g_mock_shm.completed, false);
    atomic_store(&g_mock_shm.swapped, false);
    atomic_store(&g_mock_shm.stop_flag, false);
}

void mock_shm_write_swap_cmd(const char *pod_uid, int vnpu_id, int action)
{
    strncpy(g_mock_shm.target_pod_uid, pod_uid, 63);
    g_mock_shm.target_vnpu_id = vnpu_id;
    g_mock_shm.action = action;
    atomic_store(&g_mock_shm.completed, false);
}

void mock_shm_read_swap_cmd(char *pod_uid, int *vnpu_id, int *action, bool *completed)
{
    if (pod_uid)
        strncpy(pod_uid, g_mock_shm.target_pod_uid, 63);
    if (vnpu_id)
        *vnpu_id = g_mock_shm.target_vnpu_id;
    if (action)
        *action = g_mock_shm.action;
    if (completed)
        *completed = atomic_load(&g_mock_shm.completed);
}

void mock_shm_ack_swap_cmd(void)
{
    atomic_store(&g_mock_shm.completed, true);
}

void mock_shm_set_swap_state(bool swapped, uint64_t offset, uint64_t size)
{
    atomic_store(&g_mock_shm.swapped, swapped);
    g_mock_shm.swap_offset = offset;
    g_mock_shm.swap_size = size;
}

bool mock_shm_get_swapped(void)
{
    return atomic_load(&g_mock_shm.swapped);
}

void mock_shm_set_stop_flag(bool stop)
{
    atomic_store(&g_mock_shm.stop_flag, stop);
}

bool mock_shm_get_stop_flag(void)
{
    return atomic_load(&g_mock_shm.stop_flag);
}