Name:       vcann-runtime
Version:    %{version}
Release:    1%{?dist}
Summary:    vcann-runtime Library

License:    Proprietary
Source0:    %{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  make
Requires:       Ascend

%define debug_package %{nil}

%description
vcann-runtime library

# ==============================================================
# prep
# ==============================================================
%prep
%autosetup -n %{name}-%{version}

# ==============================================================
# build
# ==============================================================
%build

BUILD_PATH=%{_topdir}/../build
if [ ! -d "${BUILD_PATH}" ]; then
    mkdir -p "${BUILD_PATH}"
fi

cd "${BUILD_PATH}"
if ! /usr/bin/cmake ..; then
     echo "make_build:cmake failed."
     exit 1
fi

if ! make -j $(nproc); then
     echo "make_build:make failed."
     exit 1
fi

# ==============================================================
# install
# ==============================================================
%install
rm -rf %{buildroot}

mkdir -p %{buildroot}/opt/enpu/vcann-rt/{lib,tools}
install -d %{buildroot}"/opt/enpu/vcann-rt/lib/"
cp -ar %{_topdir}/../build/libvruntime.so* %{buildroot}"/opt/enpu/vcann-rt/lib/"
install -m 500 %{_topdir}/../build/enpu-monitor %{buildroot}"/opt/enpu/vcann-rt/tools/"

# ==============================================================
# clean
# ==============================================================
%clean
rm -rf %{_topdir}/../build/libvruntime.so*
rm -rf %{_topdir}/../build/enpu-monitor

# ==============================================================
# files
# ==============================================================
%files
%defattr(-,root,root,-)
%attr(400, root, root) /opt/enpu/vcann-rt/lib/libvruntime.so*
%attr(500, root, root) /opt/enpu/vcann-rt/tools/enpu-monitor