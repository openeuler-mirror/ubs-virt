Name:   ubs-virt-ovs
Version:    %{version}
Release:    %{release}%{?dist}
Summary:    UBS Virt OVS Service

License:    Proprietary
Source0:    %{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires: libboundscheck
Requires: libboundscheck
Requires(post): systemd
Requires(preun):    systemd

%description
UBS Virt OVS Service

# ==============================================================
# prep
# ==============================================================
%prep
%autosetup -n %{name}-%{version}

# ==============================================================
# build
# ==============================================================
%build
mkdir -p build
cd build

# cross compile variables
%{!?cross_compile_prefix:%global cross_compile_prefix}

cmake .. \
    -DCMAKE_BUILD_TYPE=%{cmake_build_type} \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_C_COMPILER=${cross_compile_prefix}gcc \
    -DCMAKE_CXX_COMPILER=${cross_compile_prefix}g++

cmake --build . -j$(nproc)

# ==============================================================
# install
# ==============================================================
%install
rm -rf %{buildroot}

cd build
cmake --install . --prefix=%{buildroot}/usr

# install systemd unit
install -D -m 0644 \
 %{_builddir}/%{name}-%{version}/build/ubs-virt-ovs.service \
 %{buildroot}%{_unitdir}/ubs-virt-ovs.service

#install LICENSE
mkdir -p %{buildroot}/usr/share/licenses/%{name}/
install -m 0644 %{_builddir}/%{name}-%{version}/LICENSE \
 %{buildroot}/usr/share/licenses/%{name}/LICENSE

# ==============================================================
# hooks
# ==============================================================
%post
%systemd_post ubs-virt-ovs.service
systemctl enable ubs-virt-ovs > /dev/null 2>&1 || :
systemctl start ubs-virt-ovs > /dev/null 2>&1 || :

%preun
%systemd_preun ubs-virt-ovs.service

%postun
%systemd_postun_with_restart ubs-virt-ovs.service

# ==============================================================
# files
# ==============================================================
%files
%license /usr/share/licenses/%{name}/LICENSE
/usr/bin/ubs-virt-ovs
%{_unitdir}/ubs-virt-ovs.service
