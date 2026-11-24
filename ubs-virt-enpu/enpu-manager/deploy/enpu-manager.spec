# enpu-manager RPM Spec
# Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
# Licensed under Mulan PSL v2.

%global _hardened_build 1
%define debug_package %{nil}
%global enpu_manager_dir /opt/enpu/enpu-manager

# ====================================================================
# Package Information
# ====================================================================
Name:           enpu-manager
Version:        1.0.0
Release:        1%{?dist}
Summary:        Ascend NPU Management System

License:        MulanPSL-2.0
URL:            https://github.com/enpu-manager
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.15
BuildRequires:  gcc >= 9.3
BuildRequires:  gcc-c++ >= 9.3
BuildRequires:  systemd
BuildRequires:  cjson-devel
BuildRequires:  libboundscheck
Requires:       cjson
Requires(post): systemd
Requires(preun): systemd
Requires(postun): systemd

# NOTE: CANN Toolkit + Driver are NOT declared as RPM dependencies.

%description
ENPU Manager is a management system for Huawei Ascend NPU devices.
It supports device discovery, vNPU partitioning, elastic memory management,
memory swapping, and both Kubernetes and standalone deployment scenarios.

%prep
%setup -q

%build
# Step 1: Build core shared library
pushd enpu-manager/core
bash make_build.sh
popd

# Step 2: Build standalone binaries (enpu-manager + enpu-cli)
pushd enpu-manager/standalone
echo "[INFO] Building standalone binaries..."
ENPU_MANAGER_RUNPATH='$ORIGIN/../lib' bash build.sh
popd

# Step 3: Build vcann-rt (libvruntime.so + enpu-monitor)
pushd vcann-rt
echo "[INFO] Building vcann-rt..."
bash make_build.sh
popd

%install
# Preserve the permissions set by the build for executable and library artifacts.
install -d %{buildroot}%{enpu_manager_dir}/bin %{buildroot}%{enpu_manager_dir}/lib
cp --preserve=mode enpu-manager/standalone/build/output/enpu-manager \
    enpu-manager/standalone/build/output/enpu-cli %{buildroot}%{enpu_manager_dir}/bin/

# Shared library — versioned and soversion symlinks
cp --preserve=mode enpu-manager/core/build/output/libenpu_manager.so.1.0.0 %{buildroot}%{enpu_manager_dir}/lib/
ln -sf libenpu_manager.so.1.0.0 %{buildroot}%{enpu_manager_dir}/lib/libenpu_manager.so.1

# Configuration file
install -D enpu-manager/config/enpu-manager.conf %{buildroot}%{_sysconfdir}/enpu/enpu-manager.conf

# Systemd service unit
install -D enpu-manager/deploy/enpu-manager.service %{buildroot}%{_unitdir}/enpu-manager.service
sed -i 's|^ExecStart=[^[:space:]]*|ExecStart=%{enpu_manager_dir}/bin/enpu-manager|' \
    %{buildroot}%{_unitdir}/enpu-manager.service

# vcann-rt runtime library and monitor (for LD_PRELOAD into containers)
install -d %{buildroot}/opt/enpu/vcann-rt/lib %{buildroot}/opt/enpu/vcann-rt/tools
cp --preserve=mode vcann-rt/build/libvruntime.so %{buildroot}/opt/enpu/vcann-rt/lib/
cp --preserve=mode vcann-rt/build/enpu-monitor %{buildroot}/opt/enpu/vcann-rt/tools/

# State / log / data directories
install -d %{buildroot}%{_localstatedir}/lib/enpu-manager
install -d %{buildroot}%{_localstatedir}/log/enpu-manager
install -d %{buildroot}/opt/enpu

# ====================================================================
# Pre/Post Scripts — Security & Lifecycle
# ====================================================================

%pre
CANN_RUNTIME_PATH="%{?_ascend_home_path}"
if [ ! -f "${CANN_RUNTIME_PATH:-/usr/local/Ascend/cann}/lib64/libascendcl.so" ]; then
    echo "[WARNING] libascendcl.so not found. Please install Ascend CANN Toolkit."
fi

# Verify NPU driver runtime dependency
DRIVER_RUNTIME_PATH="%{?_enpu_ascend_driver_path}"
if [ ! -f "${DRIVER_RUNTIME_PATH:-/usr/local/Ascend}/driver/lib64/driver/libdcmi.so" ]; then
    echo "[WARNING] libdcmi.so not found. Please install Ascend NPU Driver."
fi

%post
%systemd_post enpu-manager.service
/sbin/ldconfig

%preun
# Stop & disable on uninstall (not upgrade)
if [ $1 -eq 0 ]; then
    systemctl stop enpu-manager.service >/dev/null 2>&1 || :
fi
%systemd_preun enpu-manager.service

%postun
# On upgrade, restart if was running
%systemd_postun_with_restart enpu-manager.service
/sbin/ldconfig

# ====================================================================
# File Lists
# ====================================================================
%files
%doc enpu-manager/README.md

%dir %{enpu_manager_dir}
%dir %{enpu_manager_dir}/bin
%dir %{enpu_manager_dir}/lib
%attr(-, root, root) %{enpu_manager_dir}/bin/enpu-manager
%attr(-, root, root) %{enpu_manager_dir}/bin/enpu-cli

%{enpu_manager_dir}/lib/libenpu_manager.so.1
%attr(-, root, root) %{enpu_manager_dir}/lib/libenpu_manager.so.1.0.0

%config(noreplace) %attr(0400, root, root) %{_sysconfdir}/enpu/enpu-manager.conf

%attr(0400, root, root) %{_unitdir}/enpu-manager.service

%attr(0700, root, root) %dir %{_localstatedir}/lib/enpu-manager
%attr(0700, root, root) %dir %{_localstatedir}/log/enpu-manager
%attr(0755, root, root) %dir /opt/enpu

# vcann-rt runtime library and monitor
%attr(0755, root, root) %dir /opt/enpu/vcann-rt
%attr(0755, root, root) %dir /opt/enpu/vcann-rt/lib
%attr(0755, root, root) %dir /opt/enpu/vcann-rt/tools
%attr(-, root, root) /opt/enpu/vcann-rt/lib/libvruntime.so
%attr(-, root, root) /opt/enpu/vcann-rt/tools/enpu-monitor

%changelog
* Sun Jul 19 2026 enpu-manager maintainer <maintainer@enpu-manager> - 1.0.0-1
- Initial RPM packaging
