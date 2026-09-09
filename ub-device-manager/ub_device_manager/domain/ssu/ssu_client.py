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

import asyncio
from typing import List
from ub_device_manager.app.models import SsuAllocSpaceReq, SsuPermReq, SsuInfo, VfeForSsu, NsConnectInfo, BindSsuVfeReq, \
    SsuNsStatus, UnbindSsuVfeReq
from ub_device_manager.common.wrapper import try_catch_log


class SsuClient:
    def __init__(self):
        pass

    @try_catch_log
    async def get_ns_connnect_info(self, name: str, vfe_guid: str = None) -> List[NsConnectInfo]:
        """
        Query NVMe connection information for a storage space on a specified VFE.
        """

        from ubse.ubs_engine_ssu import ubs_ssu_connect_info_get
        from ubse.models.ubs_engine_model_ssu import UbsUbVfe

        target_vfe: VfeForSsu = None
        if vfe_guid:
            ssu_vfe_list = await asyncio.to_thread(self.get_ssu_vfe_list)
            target_vfe = next(iter([vfe for vfe in ssu_vfe_list if vfe.vfe_guid == vfe_guid]), None)
        connect_info_list = await asyncio.to_thread(
            ubs_ssu_connect_info_get,
            name, UbsUbVfe(**target_vfe.model_dump()) if target_vfe else None)

        ns_connect_info_list: List[NsConnectInfo] = [NsConnectInfo.model_validate(connect_info, from_attributes=True)
                                                     for connect_info in connect_info_list]
        return ns_connect_info_list

    @try_catch_log
    async def show_ssu_ns_status(self, name: str = "123") -> List[SsuNsStatus]:
        """
        Get namespace information for a storage space.
        """
        from ubse.ubs_engine_ssu import ubs_ssu_ns_stats_get

        return await asyncio.to_thread(ubs_ssu_ns_stats_get, name)

    @try_catch_log
    async def bind_ssu_vfe_bus(self, body: BindSsuVfeReq) -> None:
        """
        TODO: The interface definition is incomplete.
        Bind a specified VFE to a bus.
        """
        return await asyncio.to_thread()

    @try_catch_log
    async def get_ssu_vfe_list(self) -> List[VfeForSsu]:
        """
        List all SSU-specific VFEs.
        """
        from ubse.ubs_engine_ssu import ubs_ssu_fe_device_list

        return await asyncio.to_thread(ubs_ssu_fe_device_list)

    @try_catch_log
    async def ssu_alloc(self, body: SsuAllocSpaceReq) -> SsuInfo:
        """
        Allocate SSU storage space.
        """
        from ubse.ubs_engine_ssu import ubs_ssu_space_alloc
        from ubse.ubs_engine_model_ssu import UbsSsuAllocSpaceReq

        raw_result = await asyncio.to_thread(ubs_ssu_space_alloc, UbsSsuAllocSpaceReq(**body.model_dump()))
        ssu_alloc_result = SsuInfo.model_validate(raw_result, from_attributes=True)

        return ssu_alloc_result

    @try_catch_log
    async def ssu_free(self, ssu_name: str) -> None:
        from ubse.ubs_engine_ssu import ubs_ssu_space_free

        await asyncio.to_thread(ubs_ssu_space_free, ssu_name)

    @try_catch_log
    async def get_ssu_list(self) -> List[SsuInfo]:
        """
        List all allocated storage spaces.
        """
        from ubse.ubs_engine_ssu import ubs_ssu_alloc_info_list

        raw_result = await asyncio.to_thread(ubs_ssu_alloc_info_list)
        result = [SsuInfo.model_validate(result, from_attributes=True) for result in raw_result]
        return result

    @try_catch_log
    async def show_ssu_alloc_info(self, name: str) -> SsuInfo:
        """
        Get allocated storage space information by name.
        """
        from ubse.ubs_engine_ssu import ubs_ssu_alloc_info_get

        raw_result = await asyncio.to_thread(ubs_ssu_alloc_info_get, name)
        result = SsuInfo.model_validate(raw_result, from_attributes=True)
        return result

    @try_catch_log
    async def add_ssu_access_perm(
            self,
            params: SsuPermReq,
    ) -> None:
        """
        Add SSU access permission.
        """
        from ubse.ubs_engine_ssu import ubs_ssu_access_permission_add

        ssu_perm_params = params.model_dump()
        await asyncio.to_thread(ubs_ssu_access_permission_add, **ssu_perm_params)

    @try_catch_log
    async def remove_ssu_access_perm(
            self,
            params: SsuPermReq,
    ) -> None:
        """
        Remove SSU access permission.
        """
        from ubse.ubs_engine_ssu import ubs_ssu_access_permission_remove

        ssu_perm_params = params.model_dump()
        await asyncio.to_thread(ubs_ssu_access_permission_remove, **ssu_perm_params)

    @try_catch_log
    async def unbind_ssu_vfe_bus(self, body: UnbindSsuVfeReq):
        """
        Unbind a specified VFE from a bus GUID.
        """
        from ubse.ubs_engine_ssu import ubs_ssu_vfe_unbind

        await asyncio.to_thread(ubs_ssu_vfe_unbind, **body.model_dump())
