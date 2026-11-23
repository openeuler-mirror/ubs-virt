/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 */
#ifndef ENPU_DCMI_H
#define ENPU_DCMI_H

#ifdef __cplusplus
extern "C" {
#endif

/* DCMI ABI used by EN. Keep the layouts and signatures in sync with
 * cann/driver src/custom/include/dcmi_interface_api.h (d04bdcb1fb1c).
 */
struct dcmi_chip_info {
    unsigned char chip_type[32];
    unsigned char chip_name[32];
    unsigned char chip_ver[32];
    unsigned int aicore_cnt;
};

struct dcmi_chip_info_v2 {
    unsigned char chip_type[32];
    unsigned char chip_name[32];
    unsigned char chip_ver[32];
    unsigned int aicore_cnt;
    unsigned char npu_name[32];
};

struct dcmi_die_id {
    unsigned int soc_die[5];
};

struct dcmi_hbm_info {
    unsigned long long memory_size;
    unsigned int freq;
    unsigned long long memory_usage;
    int temp;
    unsigned int bandwith_util_rate;
};

struct dcmi_proc_mem_info {
    int proc_id;
    unsigned long proc_mem_usage;
};

enum dcmi_die_type
{
    NDIE = 0,
    VDIE = 1,
    DDIE = 2,
    INVALID_DIE = 3,
};

enum
{
    DCMI_OK = 0,
    DCMI_ERR_CODE_NOT_SUPPORT_IN_CONTAINER = -8013,
    DCMI_ERR_CODE_NOT_SUPPORT = -8255,
};

#define ENPU_DCMI_SYMBOLS(X)                                                                                     \
    X(dcmi_init, (void))                                                                                         \
    X(dcmi_get_all_device_count, (int *count))                                                                   \
    X(dcmi_get_card_id_device_id_from_logicid, (int *card_id, int *device_id, unsigned int logic_id))            \
    X(dcmi_get_card_id_device_id_from_phyid, (int *card_id, int *device_id, unsigned int phy_id))                \
    X(dcmi_get_device_logic_id, (int *logic_id, int card_id, int device_id))                                     \
    X(dcmi_get_device_chip_info, (int card_id, int device_id, struct dcmi_chip_info *info))                      \
    X(dcmi_get_device_chip_info_v2, (int card_id, int device_id, struct dcmi_chip_info_v2 *info))                \
    X(dcmi_get_device_die_v2, (int card_id, int device_id, enum dcmi_die_type type, struct dcmi_die_id *die_id)) \
    X(dcmi_get_device_hbm_info, (int card_id, int device_id, struct dcmi_hbm_info *info))                        \
    X(dcmi_get_device_resource_info, (int card_id, int device_id, struct dcmi_proc_mem_info *info, int *count))  \
    X(dcmi_get_device_utilization_rate, (int card_id, int device_id, int type, unsigned int *rate))              \
    X(dcmiv2_init, (void))                                                                                       \
    X(dcmiv2_get_all_device_count, (int *count))                                                                 \
    X(dcmiv2_get_device_list, (int *device_list, int *count, int list_len))                                      \
    X(dcmiv2_get_device_hbm_info, (int device_id, struct dcmi_hbm_info *info))                                   \
    X(dcmiv2_get_device_chip_info, (int device_id, struct dcmi_chip_info_v2 *info))                              \
    X(dcmiv2_get_device_die_id, (int device_id, enum dcmi_die_type type, struct dcmi_die_id *die_id))            \
    X(dcmiv2_get_device_proc_mem_info, (int device_id, struct dcmi_proc_mem_info *info, int *count))             \
    X(dcmiv2_get_device_utilization_rate, (int device_id, int type, unsigned int *rate))

typedef struct {
#define ENPU_DCMI_FIELD(name, args) int(*name) args;
    ENPU_DCMI_SYMBOLS(ENPU_DCMI_FIELD)
#undef ENPU_DCMI_FIELD
} enpu_dcmi_api_t;

/* The returned API is immutable. Individual symbols can be absent on a given Driver. */
const enpu_dcmi_api_t *enpu_dcmi_get_api(const char **error);

#ifdef ENPU_DCMI_TEST_STUBS
#define ENPU_DCMI_DECLARATION(name, args) int name args;
ENPU_DCMI_SYMBOLS(ENPU_DCMI_DECLARATION)
#undef ENPU_DCMI_DECLARATION
#endif

#ifdef __cplusplus
}
#endif
#endif
