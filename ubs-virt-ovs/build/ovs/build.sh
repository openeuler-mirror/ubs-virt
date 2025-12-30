#!/usr/bin/env bash
set -e

PROJECT_NAME=ubs-virt-ovs
VERSION=1.0.0
RELEASE=1

# ============ build type ============
BUILD_TYPE=${1:-relwithdebinfo}

case "${BUILD_TYPE}" in
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
  *)
    echo "Usage: $0 [debug|release|relwithdebinfo]"
    exit 1
    ;;
esac

# ============ paths ============
ROOT_DIR=$(cd "$(dirname "$0")/../.." && pwd)
RPMBUILD_DIR=${ROOT_DIR}/build/ovs/rpmbuild
OUTPUT_DIR=${ROOT_DIR}/build/ovs/output

echo "[INFO] Build type: ${CMAKE_BUILD_TYPE}"
echo "[INFO] Project root: ${ROOT_DIR}"

rm -rf "${RPMBUILD_DIR}"
mkdir -p "${RPMBUILD_DIR}"/{BUILD,RPMS,SOURCES,SPECS,SRPMS} "${OUTPUT_DIR}"

# ============ source tar ============
tar czf "${RPMBUILD_DIR}/SOURCES/${PROJECT_NAME}-${VERSION}.tar.gz" \
  --exclude=build \
  --exclude=.git \
  -C "${ROOT_DIR}" \
  --transform "s,^,${PROJECT_NAME}-${VERSION}/," .

# ============ spec ============
cp "${ROOT_DIR}/build/ovs/ubs-virt-ovs.spec" \
   "${RPMBUILD_DIR}/SPECS/${PROJECT_NAME}.spec"

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

# ============ collect RPMs ============
find "${RPMBUILD_DIR}/RPMS" -name "*.rpm" -exec cp -v {} "${OUTPUT_DIR}" \;

echo "[INFO] Output RPMs:"
ls -lh "${OUTPUT_DIR}"