#!/bin/bash
set -e
PROJECT_NAME=vcann-runtime
VERSION=1.0

CURRENT_PATH=$(cd "$(dirname "$0")"; pwd)

if [ -z "$ASCEND_HOME_PATH" ] || [ -z "$ASCEND_TOOLKIT_HOME" ]; then
    echo "[ERROR] ASCEND_HOME_PATH OR ASCEND_TOOLKIT_HOME is not set!"
    exit 1
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
cp "${ROOT_DIR}/vruntime.spec" \
   "${RPMBUILD_DIR}/SPECS/${PROJECT_NAME}.spec"

# ========== build RPM =========
echo "Building RPM package..."
rpmbuild --define "_topdir ${RPMBUILD_DIR}" --define "version ${VERSION}" -ba "${RPMBUILD_DIR}/SPECS/${PROJECT_NAME}.spec"

# ========== collect RPMs =========
echo "[INFO] Output RPMs:"
find "${RPMBUILD_DIR}/RPMS" -name "*.rpm" -type f -exec ls -lh {} \;
