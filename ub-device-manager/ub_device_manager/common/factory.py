# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
import time

import uuid
from contextlib import asynccontextmanager

from http import HTTPStatus

from fastapi import FastAPI, Request, HTTPException
from loguru import logger
from starlette.responses import JSONResponse

from ub_device_manager.app.vm_app import app as vm_app
from ub_device_manager.app.npu_app import app as npu_app

# TODO: SSU API is not ready.
# from ub_device_manager.app.ssu_app import app as ssu_app
from ub_device_manager.common.logger import REQUEST_ID_VAR

_TITLE = "Ub device manager"
_VERSION = "1.0.0"
_DESCRIPTION = ""


@asynccontextmanager
async def lifespan(app: FastAPI):
    logger.info("Global service initialization completed; Loguru is configured.")
    yield
    logger.info("Service is shutting down gracefully.")


def get_logger(request: Request):
    return request.state.request_logger


def create_app() -> FastAPI:
    new_app = FastAPI(
        title=_TITLE,
        version=_VERSION,
        description=_DESCRIPTION,
        lifespan=lifespan
    )

    @new_app.middleware("http")
    async def add_process_time_header(request: Request, call_next):
        start_time = time.time()

        logger.info(f"Received request: {request.method} {request.url.path}")

        response = await call_next(request)

        process_time = time.time() - start_time
        logger.info(f"Request completed: {request.method} {request.url.path} | duration: {process_time:.4f}s")

        response.headers["X-Process-Time"] = str(process_time)
        return response

    @new_app.middleware("http")
    async def request_id_middleware(request: Request, call_next):
        req_id = str(uuid.uuid4())
        token = REQUEST_ID_VAR.set(req_id)
        try:
            logger.info(f"Start handle request: {request.method} {request.url.path}")
            response = await call_next(request)
            logger.info(f"Finish handle request, status_code={response.status_code}")

            response.headers["X-Request-ID"] = req_id

            return response
        finally:
            REQUEST_ID_VAR.reset(token)

    @new_app.exception_handler(HTTPException)
    async def http_exception_handler(request: Request, exc: HTTPException):
        # exc.status_code is the HTTP status code and exc.detail is the error message.
        logger.error(
            f"Request failed: {request.method} {request.url.path}, "
            f"code: {exc.status_code}, detail: {exc.detail}")

        return JSONResponse(
            content=exc.detail,
            status_code=exc.status_code
        )

    @new_app.exception_handler(Exception)
    async def global_exception_handler(request: Request, exc: Exception):
        error_msg = f"Unhandled exception: {request.method} {request.url.path}, error: {exc}"
        logger.error(error_msg)

        return JSONResponse(
            content=error_msg,
            status_code=HTTPStatus.INTERNAL_SERVER_ERROR
        )

    new_app.router.routes.extend(vm_app.routes + npu_app.routes)
    # TODO: SSU API is not ready.
    # new_app.router.routes.extend(vm_app.routes + npu_app.routes + ssu_app.routes)

    return new_app


final_app = create_app()
