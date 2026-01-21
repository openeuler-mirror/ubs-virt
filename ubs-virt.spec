Name:   ubs-virt
Version:    %{version}
Release:    %{release}%{?dist}
Summary:    ubs-virt Test

License:    Proprietary
Source0:    %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++ gcc cmake make
BuildRequires:  patch

%description
ubs-virt Test

# ==============================================================
# prep
# ==============================================================
%prep
%autosetup -n %{name}-%{version}

# ==============================================================
# build
# ==============================================================
%build
bash build.sh
