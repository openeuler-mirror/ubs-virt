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
%global debug_package %{nil}
%global __os_install_post %{nil}

%if %{defined python_v}
    %define python_version %{python_v}
%else
    %define python_version 3.11
%endif
%define python_path python%{python_version}
%define site_packages_dir /usr/lib/%{python_path}/site-packages
%define service_user ub-device-manager

Name:           ub-device-manager
Version:        1.0.0
Release:        1%{?dist}
Summary:        Ub Device Manager
License:        Mulan PSL v2
Source0:        %{name}-%{version}.tar.gz
BuildArch:      aarch64
Requires:       python3

%description
UB Device Manager: 在本地节点创建/删除 UB(NPU 直通)虚机、使能/去使能/查询 UB 设备。
提供 HTTP、Python SDK(ub_device_manager.ub_dm_sdk) 与 ubdmctl CLI 三种入口。
Python 依赖(fastapi/uvicorn/pydantic/loguru/httpx/typer/rich/PyYAML 等)需另行安装，
参见包内 requirements.txt。

%prep
%setup -c

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}/etc/ub_device_manager
mkdir -p %{buildroot}/var/log/ub_device_manager
mkdir -p %{buildroot}/etc/systemd/system
mkdir -p %{buildroot}%{site_packages_dir}/ub_device_manager

# 清理构建机上的缓存文件
find . -name "__pycache__" -type d -prune -exec rm -rf {} +
find . -name "*.pyc" -type f -delete

# 服务端主包与内置 Python SDK
cp -r ub_device_manager/* %{buildroot}%{site_packages_dir}/ub_device_manager/
cp requirements.txt %{buildroot}%{site_packages_dir}/ub_device_manager/requirements.txt

# 配置文件: 当前代码从 site-packages 包内读取 toml, /etc 下为运维预留副本
cp ub_device_manager/ub_device_manager.toml %{buildroot}/etc/ub_device_manager/ub_device_manager.toml

# 服务模板/spec 不随包安装到 site-packages
rm -f %{buildroot}%{site_packages_dir}/ub_device_manager/ub-device-manager.service
rm -f %{buildroot}%{site_packages_dir}/ub_device_manager/ub_device_manager.spec

# 渲染 systemd 服务文件中的 Python 与安装路径占位符
sed -e "s|@PYTHON3@|%{_bindir}/python%{python_version}|g" \
    -e "s|@SITE_PACKAGES@|%{site_packages_dir}|g" \
    ub_device_manager/ub-device-manager.service > %{buildroot}/etc/systemd/system/ub-device-manager.service

# 生成 ubdmctl 全局命令(指向 CLI 控制器)
cat << 'EOF' > %{buildroot}%{_bindir}/ubdmctl
#!/bin/bash
exec %{_bindir}/python%{python_version} %{site_packages_dir}/ub_device_manager/ub_device_manager_cli.py "$@"
EOF

chmod +x %{buildroot}%{_bindir}/ubdmctl

%pre
getent group %{service_user} >/dev/null || groupadd -r %{service_user}
getent passwd %{service_user} >/dev/null || \
    useradd -r -g %{service_user} -d / -s /sbin/nologin \
    -c "Ub Device Manager Service User" %{service_user}

# 服务用户需要访问 libvirt 与 UBSE, 相关组存在时加入
for grp in libvirt qemu ubse; do
    if getent group "$grp" >/dev/null; then
        usermod -aG "$grp" %{service_user} || true
    fi
done
exit 0

%post
systemctl enable ub-device-manager
systemctl daemon-reload >/dev/null 2>&1 || true
chown -R %{service_user}:%{service_user} /var/log/ub_device_manager >/dev/null 2>&1 || true

%preun
if [ "$1" -eq 0 ]; then
    systemctl stop ub-device-manager >/dev/null 2>&1 || true
    systemctl disable ub-device-manager >/dev/null 2>&1 || true
fi

%postun
systemctl daemon-reload >/dev/null 2>&1 || true

%clean
rm -rf %{buildroot}

%files
%attr(0750, %{service_user}, %{service_user}) %{site_packages_dir}/ub_device_manager
%attr(0644, root, root) /etc/systemd/system/ub-device-manager.service
%dir %attr(0750, %{service_user}, %{service_user}) /etc/ub_device_manager
%attr(0640, %{service_user}, %{service_user}) /etc/ub_device_manager/ub_device_manager.toml
%dir %attr(0750, %{service_user}, %{service_user}) /var/log/ub_device_manager
%attr(0755, root, root) %{_bindir}/ubdmctl

%changelog
* Tue Jun 30 2026 Developer - 1.0.0
- 重构安装路径，支持动态 Python 版本传参，并生成 ubdmctl 全局控制命令
