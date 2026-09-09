##########################################################################################################
# Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
#
# ub-device-manager is licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
##########################################################################################################
from typing import Optional

from pydantic import BaseModel, ConfigDict, Field


class CreateVmRequest(BaseModel):
    model_config = ConfigDict(extra="forbid")

    xml_text: Optional[str] = None
    xml_path: Optional[str] = None
    upi: str
    need_nic: bool = False
    npu_ids: Optional[list[str]] = None
    npu_count: Optional[int] = Field(None, ge=0)


class BindNpuDeviceRequest(BaseModel):
    upi: str
    ids: Optional[list[str]] = None
    count: Optional[int] = Field(None, ge=1)
    need_nic: bool = False


class UnbindNpuDeviceRequest(BaseModel):
    bus_guid: str


class BoundUbDevice(BaseModel):
    id: str
    guid: str
    type: str


class NpuDeviceInfo(BaseModel):
    id: str
    guid: str
    bus_guid: Optional[str] = None
