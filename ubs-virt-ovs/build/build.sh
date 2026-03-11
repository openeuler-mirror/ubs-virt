#!/usr/bin/env bash
set -e

PROJECT_NAME=ubs-virt-ovs
VERSION=1.0.0
RELEASE=1

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)

# ============ build type ============
BUILD_TYPE=${1:-relwithdebinfo}
BUILD_TYPE_LOWER=$(echo "${BUILD_TYPE}" | tr '[:upper:]' '[:lower:]')

case "${BUILD_TYPE_LOWER}" in
  debug)
    CMAKE_BUILD_TYPE=Debug
    RPM_RELEASE_SUFFIX=.debug
    ;;
  release)
    CMAKE_BUILD_TYPE=Release
    RPM_RELEASE_SUFFIX=
    ;;
  relwithdebinfo)
    CMAKE_BUILD_TYPE=RelwithDebinfo
    RPM_RELEASE_SUFFIX=.rel
    ;;
  dt)
    cd "${ROOT_DIR}"
    bash "${SCRIPT_DIR}/run_ut.sh"
    exit 0
    ;;
  *)
    echo "Usage: $0 [debug|release|relwithdebinfo]"
    exit 1
    ;;
esac

# ============ paths ============
RPMBUILD_DIR="${ROOT_DIR}/build/rpmbuild"
OUTPUT_DIR="${ROOT_DIR}/build/output"
SPEC_FILE="${SCRIPT_DIR}/${PROJECT_NAME}.spec"

[[ -f "${SPEC_FILE}" ]] || {
  echo "[ERROR] spec file not found: ${SPEC_FILE}"
  exit 1
}

[[ -f "${ROOT_DIR}/CMakeLists.txt" ]] || {
  echo "[ERROR] not a valid project root: ${ROOT_DIR}"
  exit 1
}

echo "[INFO] Build type: ${CMAKE_BUILD_TYPE}"
echo "[INFO] Project root: ${ROOT_DIR}"
echo "[INFO] Spec file: ${SPEC_FILE}"

rm -rf "${RPMBUILD_DIR}"
mkdir -p "${RPMBUILD_DIR}"/{BUILD,RPMS,SOURCES,SPECS,SRPMS} "${OUTPUT_DIR}"

# ============ create source tar ============
SRC_TAR="${RPMBUILD_DIR}/SOURCES/${PROJECT_NAME}-${VERSION}.tar.gz"
cp "${ROOT_DIR}/../LICENSE" "${ROOT_DIR}/LICENSE"

# Go to project root
cd "${ROOT_DIR}"/..
tar czf "${SRC_TAR}" \
  --exclude=build/rpmbuild \
  --exclude=build/output \
  --exclude=.git \
  --transform "s,^ubs-virt-ovs,${PROJECT_NAME}-${VERSION}," \
  ubs-virt-ovs

rm "${ROOT_DIR}/LICENSE"

echo "[INFO] Source tar created: ${SRC_TAR}"

# ============ copy spec file ============
cp "${SPEC_FILE}" "${RPMBUILD_DIR}/SPECS/${PROJECT_NAME}.spec"

# ============ cross compile options ============
# Set the cross-compilation prefix (can be adjusted according to the target platform)
CROSS_COMPILE=${CROSS_COMPILE:-aarch64-linux-gnu-}

# ============ build RPM ============
rpmbuild \
  --define "_topdir ${RPMBUILD_DIR}" \
  --define "version ${VERSION}" \
  --define "release ${RELEASE}${RPM_RELEASE_SUFFIX}" \
  --define "cmake_build_type ${CMAKE_BUILD_TYPE}" \
  --define "cross_compile_prefix ${CROSS_COMPILE}" \
  -ba "${RPMBUILD_DIR}/SPECS/${PROJECT_NAME}.spec"

# ============ collect built RPMs ============
find "${RPMBUILD_DIR}/RPMS" -name "*.rpm" -exec cp -v {} "${OUTPUT_DIR}" \;

echo "[INFO] Output RPMs:"
ls -lh "${OUTPUT_DIR}"