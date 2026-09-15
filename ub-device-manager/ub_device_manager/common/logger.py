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
from loguru import logger
import logging
import sys

from ub_device_manager.common.config import CONFIG
from contextvars import ContextVar

REQUEST_ID_VAR = ContextVar("request_id", default="")
LOG_PATH = "/var/log/ub_device_manager/ub_device_manager.log"


class InterceptHandler(logging.Handler):
    def emit(self, record):
        try:
            level = logger.level(record.levelname).name
        except ValueError:
            level = record.levelno

        frame, depth = logging.currentframe(), 2
        while frame.f_code.co_filename == logging.__file__:
            frame = frame.f_back
            depth += 1

        logger.opt(depth=depth, exception=record.exc_info).log(level, record.getMessage())


def request_id_filter(record):
    req_id = REQUEST_ID_VAR.get()
    record["extra"]["request_id"] = req_id if req_id else "-"
    if record["name"] == "logging" and record["function"] == "callHandlers":
        return False
    return True


def setup_logging():
    logging.getLogger("uvicorn.error").disabled = True
    logger.remove()

    logger.add(
        sink=LOG_PATH,
        backtrace=False,
        diagnose=False,
        rotation=CONFIG.get("log", {}).get("max_file_size", "10 MB"),
        retention=CONFIG.get("log", {}).get("max_file_count", 10),
        compression="zip",
        level=CONFIG.get("log", {}).get("level", "INFO"),
        enqueue=True,
        format="<green>{time:YYYY-MM-DD HH:mm:ss}</green> | <level>{level: <8}</level> | "
               "<magenta>{extra[request_id]}</magenta> | <blue>{name}:{function}:{line}</blue> - <level>{message}</level>"
        , filter=request_id_filter)

    if CONFIG.get("log", {}).get("stdout", False):
        logger.add(
            sink=sys.stdout,
            level="INFO",
            enqueue=True,
            format="<green>{time:YYYY-MM-DD HH:mm:ss}</green> | <level>{level: <8}</level> | "
                   "<magenta>{extra[request_id]}</magenta> | <blue>{name}:{function}:{line}</blue> - <level>{message}</level>"
            , filter=request_id_filter
        )
    # 4. Intercept Uvicorn and FastAPI logs.
    # Replace handlers on common loggers with InterceptHandler.
    seen = set()
    for name in logging.root.manager.loggerDict.keys():
        if name.startswith("uvicorn") or name.startswith("fastapi"):
            _logger = logging.getLogger(name)
            _logger.handlers = [InterceptHandler()]
            seen.add(name)

    logging.getLogger().handlers = [InterceptHandler()]
