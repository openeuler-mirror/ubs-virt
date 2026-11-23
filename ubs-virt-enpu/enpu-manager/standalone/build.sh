#!/bin/bash
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
#
# enpu-manager-standalone is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#          http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.

set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
CORE_DIR="${SCRIPT_DIR}/../core"
CORE_LIB="${CORE_DIR}/build/output/libenpu_manager.so"
OUTPUT_DIR="${SCRIPT_DIR}/build/output"

CC=${CC:-gcc}
CFLAGS=(-Wall -O2 -fPIC)
RUNPATH=${ENPU_MANAGER_RUNPATH:-'$ORIGIN/../../../core/build/output'}

INCLUDES=(-I"${CORE_DIR}/include" -I"${SCRIPT_DIR}/api" -I"${SCRIPT_DIR}/../../common/include")
LDFLAGS=(-L"${CORE_DIR}/build/output"
    -Wl,-rpath,"${RUNPATH}"
    -lenpu_manager -lcjson -lpthread)

if [[ ! -f "${CORE_LIB}" ]]; then
    echo "[ERROR] Core library not found: ${CORE_LIB}"
    echo "[INFO] Build core first: cd ../core && bash make_build.sh"
    exit 1
fi

# Reuse the exact securec header/library selected by the core CMake build.
CORE_CACHE="${CORE_DIR}/build/CMakeCache.txt"
SECUREC_INCLUDE=$(sed -n 's/^C_SEC_BOUNDSCHECK_INCLUDE:PATH=//p' "${CORE_CACHE}")
SECUREC_LIBRARY=$(sed -n 's/^C_SEC_BOUNDSCHECK_LIBRARY:FILEPATH=//p' "${CORE_CACHE}")
if [[ ! -f "${SECUREC_INCLUDE}/securec.h" || ! -f "${SECUREC_LIBRARY}" ]]; then
    echo "[ERROR] securec dependency is missing; configure and build core first"
    exit 1
fi
INCLUDES+=(-I"${SECUREC_INCLUDE}")
LDFLAGS+=("${SECUREC_LIBRARY}")

mkdir -p "${OUTPUT_DIR}"
echo "[INFO] Using compiler: ${CC}"
echo "[INFO] Core library: ${CORE_LIB}"

echo "[INFO] Building enpu-manager..."
"${CC}" "${CFLAGS[@]}" "${INCLUDES[@]}" \
    "${SCRIPT_DIR}/api/rest_api.c" \
    "${SCRIPT_DIR}/api/resource_format.c" \
    "${SCRIPT_DIR}/cmd/enpu_manager_main.c" \
    -o "${OUTPUT_DIR}/enpu-manager" \
    "${LDFLAGS[@]}"

if [[ $? -ne 0 ]]; then
    echo "[ERROR] Failed to build enpu-manager"
    exit 1
fi
echo "[OK] enpu-manager -> ${OUTPUT_DIR}/enpu-manager"

echo "[INFO] Building enpu-cli..."
"${CC}" "${CFLAGS[@]}" -I"${SCRIPT_DIR}/cmd" -I"${SECUREC_INCLUDE}" -I"${SCRIPT_DIR}/../../common/include" \
    "${SCRIPT_DIR}/cmd/cli.c" \
    "${SCRIPT_DIR}/../../common/src/allocation_id.c" \
    -o "${OUTPUT_DIR}/enpu-cli" "${SECUREC_LIBRARY}"

if [[ $? -ne 0 ]]; then
    echo "[ERROR] Failed to build enpu-cli"
    exit 1
fi
echo "[OK] enpu-cli -> ${OUTPUT_DIR}/enpu-cli"

echo "[INFO] Build complete"
ls -la "${OUTPUT_DIR}/"
