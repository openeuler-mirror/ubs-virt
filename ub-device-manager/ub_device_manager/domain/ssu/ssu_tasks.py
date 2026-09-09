##########################################################################################################
# Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
#
# ub-device-manager is licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#      http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
##########################################################################################################

from typing import List

from ub_device_manager.app.models import (
    BindSsuVfeReq,
    Message,
    NpuDeviceInfo,
    NsConnectInfo,
    SsuAllocSpaceReq,
    SsuInfo,
    SsuNsStatus,
    SsuPermReq,
    UnbindSsuVfeReq,
    VfeForSsu,
)
from ub_device_manager.common.async_task_framework import AsyncTask, In, Out
from ub_device_manager.domain.ssu.ssu_client import SsuClient


class AllocSsuTask(AsyncTask):
    def __init__(self):
        self.ssu_client = SsuClient()

    async def execute(self, input_data: SsuAllocSpaceReq) -> SsuInfo:
        return await self.ssu_client.ssu_alloc(input_data)


class GetSsuListTask(AsyncTask):
    def __init__(self):
        self.ssu_client = SsuClient()

    async def execute(self, input_data: None) -> List[SsuInfo]:
        return await self.ssu_client.get_ssu_list()


class ShowSsuTask(AsyncTask):
    def __init__(self):
        self.ssu_client = SsuClient()

    async def execute(self, name: str) -> SsuInfo:
        return await self.ssu_client.show_ssu_alloc_info(name)


class FreeSsuSpaceTask(AsyncTask):
    def __init__(self):
        self.ssu_client = SsuClient()

    async def execute(self, ssu_name: str) -> Out:
        ssu_info = await self.ssu_client.show_ssu_alloc_info(ssu_name)

        await self.ssu_client.ssu_free(ssu_name)


class GetNsConnectInfoTask(AsyncTask):
    def __init__(self):
        self.ssu_client = SsuClient()

    async def execute(self, input_data: tuple) -> List[NsConnectInfo]:
        name, vfe_guid = input_data
        return await self.ssu_client.get_ns_connnect_info(name, vfe_guid)


class ShowSsuNsStatusTask(AsyncTask):
    def __init__(self):
        self.ssu_client = SsuClient()

    async def execute(self) -> List[SsuNsStatus]:
        return await self.ssu_client.show_ssu_ns_status()


class BindSsuVfeTask(AsyncTask):
    def __init__(self):
        self.ssu_client = SsuClient()

    async def execute(self) -> None:
        await self.ssu_client.bind_ssu_vfe_bus(None)


class GetSsuVfeListTask(AsyncTask):
    def __init__(self):
        self.ssu_client = SsuClient()

    async def execute(self) -> List[VfeForSsu]:
        return await self.ssu_client.get_ssu_vfe_list()


class AddSsuAccessPermTask(AsyncTask):
    def __init__(self):
        self.ssu_client = SsuClient()

    async def execute(self) -> None:
        await self.ssu_client.add_ssu_access_perm(None)


class RemoveSsuAccessPermTask(AsyncTask):
    def __init__(self):
        self.ssu_client = SsuClient()

    async def execute(self) -> None:
        await self.ssu_client.remove_ssu_access_perm(None)


class UnbindSsuVfeTask(AsyncTask):
    def __init__(self):
        self.ssu_client = SsuClient()

    async def execute(self) -> CommonResponse:
        return await self.ssu_client.unbind_ssu_vfe_bus(None)
