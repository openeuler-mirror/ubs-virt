#!/bin/bash
set -e
PROJECT_NAME=vcann-runtime
VERSION=1.0

CURRENT_PATH=$(cd "$(dirname "$0")"; pwd)

if [ -z "$ASCEND_HOME_PATH" ] || [ -z "$ASCEND_TOOLKIT_HOME" ]; then
    echo "[ERROR] ASCEND_HOME_PATH OR ASCEND_TOOLKIT_HOME is not set!"
    exit 1
fi

# ========== CANN Verison =========
CANN_VERSION=${1:-"8.3.0"}

IF_VALID_FORMAT='^[0-9]+(\.[0-9]+)*$'

if [[ ! "$CANN_VERSION" =~ $IF_VALID_FORMAT ]]; then
    echo "Error: Invalid version format: '$CANN_VERSION'"
    echo "Only numeric versions (e.g., 8.5 or 8.5.0) are supported."
    exit 1
fi

BASE_VERSION="8.5.0"

IS_NEW_VER=$(printf "%s\n%s" "$BASE_VERSION" "$CANN_VERSION" | sort -V | head -n1)
if [ "$IS_NEW_VER" = "$BASE_VERSION" ]; then
    #  8.5.0+
    BUILD_WITH_NEW=1
    echo "$CANN_VERSION is greater than or equal to $BASE_VERSION set BUILD_WITH_NEW to 1"
else
    #  8.4.x
    BUILD_WITH_NEW=0
    echo "$CANN_VERSION is less than $BASE_VERSION set BUILD_WITH_NEW to 0"
fi

# ========== paths =========
ROOT_DIR=${CURRENT_PATH}
RPMBUILD_DIR="${CURRENT_PATH}/rpmbuild"
rm -rf "${RPMBUILD_DIR}"
mkdir -p "${RPMBUILD_DIR}"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# ========== source tar =========
tar czf "${RPMBUILD_DIR}/SOURCES/${PROJECT_NAME}-${VERSION}.tar.gz" \
    --exclude=build \
    --exclude=.git \
    -C "${ROOT_DIR}/src" \
    --transform "s,^,${PROJECT_NAME}-${VERSION}/," .

# ========== spec =========
cp "${ROOT_DIR}/vcann-runtime.spec" \
    "${RPMBUILD_DIR}/SPECS/${PROJECT_NAME}.spec"

# ========== build RPM =========
echo "Building RPM package..."
rpmbuild --define "_topdir ${RPMBUILD_DIR}" --define "version ${VERSION}" --define "build_with_new ${BUILD_WITH_NEW}" -ba "${RPMBUILD_DIR}/SPECS/${PROJECT_NAME}.spec"

# ========== collect RPMs =========
echo "[INFO] Output RPMs:"
find "${RPMBUILD_DIR}/RPMS" -name "*.rpm" -type f -exec ls -lh {} \;
