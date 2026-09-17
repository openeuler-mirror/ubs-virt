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
from fastapi import HTTPException

from ub_device_manager.exceptions import (
    InvalidCreateVmRequest,
    UbDmHTTPException,
    VmXmlBuildFailed,
)


class TestUbDmHTTPException:
    """Basic exception hierarchy tests."""

    def test_default_detail(self):
        exc = InvalidCreateVmRequest()
        assert exc.status_code == 400
        assert exc.detail == "invalid create vm request"

    def test_custom_detail(self):
        exc = InvalidCreateVmRequest("vm name is missing")
        assert exc.detail == "vm name is missing"

    def test_exception_hierarchy(self):
        assert issubclass(UbDmHTTPException, HTTPException)
        assert issubclass(InvalidCreateVmRequest, UbDmHTTPException)
        assert issubclass(VmXmlBuildFailed, UbDmHTTPException)

    def test_subclass_code_differs(self):
        assert InvalidCreateVmRequest.code != VmXmlBuildFailed.code
