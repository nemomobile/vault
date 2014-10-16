Summary: Incremental backup/restore framework
Name: vault
Version: 0.1.0
Release: 1
License: LGPL21
Group: Development/Liraries
URL: https://github.com/nemomobile/vault
Source0: %{name}-%{version}.tar.bz2
BuildRequires: cmake >= 2.8
BuildRequires: pkgconfig(cor) >= 0.1.14
BuildRequires: pkgconfig(gittin)
BuildRequires: pkgconfig(tut) >= 0.0.3
BuildRequires: pkgconfig(Qt5Core) >= 5.2.0
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(qtaround) >= 0.2.0
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
Incremental backup/restore framework

%package devel
Summary: vault headers etc.
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
%description devel
vault library header files etc.

%package tests
Summary:    Tests for vault
License:    LGPLv2.1
Group:      System Environment/Libraries
Requires:   %{name} = %{version}-%{release}
%description tests
%summary

%prep
%setup -q

%build
%cmake -DVERSION=%{version} %{?_with_multiarch:-DENABLE_MULTIARCH=ON}
make %{?jobs:-j%jobs}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=%{buildroot}

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_libdir}/libvault-core.so*
%{_libdir}/libvault-transfer.so*
%{_libdir}/libvault-unit.so*
%{_libdir}/qt5/qml/NemoMobile/Vault/*
%{_bindir}/vault

%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/vault-unit.pc
%dir %{_includedir}/vault
%{_includedir}/vault/*.hpp

%files tests
%defattr(-,root,root,-)
/opt/tests/vault/*

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig
