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

#ifndef DCMI_INTERFACE_API_H
#define DCMI_INTERFACE_API_H

#if defined(__cplusplus)
extern "C" {
#endif

struct dcmi_proc_mem_info {
    int proc_id;
    unsigned long proc_mem_usage;
};

#define MAX_CHIP_NAME_LEN 32 /* Maximum length of chip name */

struct dcmi_chip_info {
    unsigned char chip_type[MAX_CHIP_NAME_LEN];
    unsigned char chip_name[MAX_CHIP_NAME_LEN];
    unsigned char chip_ver[MAX_CHIP_NAME_LEN];
    unsigned int aicore_cnt;
};

struct dcmi_chip_info_v2 {
    unsigned char chip_type[MAX_CHIP_NAME_LEN];
    unsigned char chip_name[MAX_CHIP_NAME_LEN];
    unsigned char chip_ver[MAX_CHIP_NAME_LEN];
    unsigned int aicore_cnt;
    unsigned char npu_name[MAX_CHIP_NAME_LEN];
};

#define DCMI_OK 0
#define DCMI_ERROR_CODE_BASE (-8000)
#define DCMI_ERR_CODE_INNER_ERR (DCMI_ERROR_CODE_BASE - 5)

int dcmi_init(void);
int dcmi_get_card_id_device_id_from_logicid(int *card_id, int *device_id, unsigned int device_logic_id);
int dcmi_get_device_resource_info(int card_id, int device_id, struct dcmi_proc_mem_info *proc_info, int *proc_num);
int dcmi_get_device_utilization_rate(int card_id, int device_id, int input_type, unsigned int *utilization_rate);
int dcmi_get_card_id_device_id_from_phyid(int *card_id, int *device_id, unsigned int device_phy_id);
int dcmi_get_device_chip_info(int card_id, int device_id, struct dcmi_chip_info *chip_info);

int dcmiv2_init(void);
int dcmiv2_get_device_utilization_rate(int dev_id, int input_type, unsigned int *utilization_rate);
int dcmiv2_get_device_proc_mem_info(int dev_id, struct dcmi_proc_mem_info *proc_info, int *proc_num);
int dcmiv2_get_device_list(int *device_list, int *device_count, int list_len);
int dcmiv2_get_device_chip_info(int dev_id, struct dcmi_chip_info_v2 *chip_info);

#if defined(__cplusplus)
}
#endif

#endif // DCMI_INTERFACE_API_H