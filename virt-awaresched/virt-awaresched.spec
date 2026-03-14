# Restore old style debuginfo creation for rpm >= 4.14.
%undefine _debugsrce_packages
%undefine _debuginfo_subpackages

# -*- rpm-spec -*-
Summary:        RPM package
Name:           virt-awaresched
Version:        1.0.0
Release:        1
Source0:        %{name}.tar.gz
License:        MIT
Vendor:         Huawei Technologies Co., Ltd.
Prefix:         /usr

BuildRequires:  gcc-c++ gcc cmake make
BuildRequires:  patch libvirt-devel libboundscheck
Requires:       libvirt
Requires:       libboundscheck
Requires:       libcap

%define _rpmdir %_topdir/RPMS
%define _srcrpmdir %_topdir/SRPMS
%define _unpackaged_files_terminate_build 0
%define project_dir %{name}


%description
virt awaresched

%define service_name vas-daemon.service


%prep
# %setup -q -T -c -n %{name}

%build
# No build steps required


%install
mkdir -p %{buildroot}/var/log/vas
mkdir -p %{buildroot}/usr/local/vas/bin
mkdir -p %{buildroot}/usr/local/bin
mkdir -p %{buildroot}/var/run/vas
ls -l  %{buildroot}/var/run
cp %{project_dir}/build/bin/vas_daemon %{buildroot}/usr/local/vas/bin/
cp %{project_dir}/build/bin/vasctl %{buildroot}/usr/local/bin/
mkdir -p %{buildroot}/usr/lib/systemd/system
cp %{project_dir}/%{service_name} %{buildroot}/usr/lib/systemd/system/


%pre
set -e
if systemctl cat %{service_name} >/dev/null 2>&1 ; then
    systemctl stop %{service_name} || true
    systemctl disable %{service_name} || true
fi


%post
set -e
systemctl daemon-reload
systemctl enable vas-daemon.service
if ! systemctl is-active --quiet vas-daemon.service; then
    systemctl start vas-daemon.service
fi


%preun
set -e
if [ "$1" -ne 0 ]; then
    echo "skip preun"
    exit 0
fi
if systemctl cat %{service_name} >/dev/null 2>&1 ; then
    systemctl stop %{service_name} || true
    systemctl disable %{service_name} || true
fi
if systemctl list-units --type=service | grep -q %{service_name}; then
    systemctl reset-failed %{service_name} || true
fi
service_file="/etc/systemd/system/vas-daemon.service"
if [ -f "$service_file" ]; then
    rm -rf $service_file
fi
systemctl daemon-reload


%postun
if [ "$1" -ne 0 ]; then
    echo "skip preun"
    exit 0
fi
if [ -d /usr/local/vas ]; then
    rm -rf /usr/local/vas
fi
if [ -d /var/run/vas ]; then
    rm -rf /var/run/vas
fi


%files
%attr(0550, root, root) %dir /usr/local/vas
%attr(0500, root, root) %dir /usr/local/vas/bin
%attr(0750, root, root) %dir /var/log/vas
%attr(0700, root, root) %dir /var/run/vas
%attr(0500, root, root) /usr/local/vas/bin/vas_daemon
%attr(0500, root, root) /usr/local/bin/vasctl
%attr(0644, root, root) /usr/lib/systemd/system/vas-daemon.service
