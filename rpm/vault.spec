%{!?_with_usersession: %{!?_without_usersession: %define _with_usersession --with-usersession}}
%{!?cmake_install: %global cmake_install make install DESTDIR=%{buildroot}}

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
BuildRequires: pkgconfig(qtaround) >= 0.2.6
%{?_with_usersession:Requires: systemd-user-session-targets}
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
Incremental backup/restore framework

%{?_with_usersession:%define _userunitdir %{_libdir}/systemd/user/}

%package -n libvault
Group: System/Libraries
Summary: Vault backup framework libraries
%description -n libvault
%summary

%package qt5-declarative
Group: System/Libraries
Summary: Vault backup framework QML plugin
# declarative part is moved from vault to the separate package
Obsoletes: vault < 0.3.0
Requires: libvault = %{version}
%description qt5-declarative
%summary

%package devel
Summary: vault headers etc.
Group: Development/Libraries
Requires: libvault = %{version}
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

%define tools_dir %{_libexecdir}/vault

%build
%cmake -DVERSION=%{version} %{?_with_multiarch:-DENABLE_MULTIARCH=ON} -DTOOLS_DIR=%{tools_dir} -DCMAKE_BUILD_TYPE=Release
make %{?jobs:-j%jobs}

%install
rm -rf $RPM_BUILD_ROOT
%cmake_install

%if 0%{?_with_usersession:1}
install -D -p -m644 tools/vault-gc.service %{buildroot}%{_userunitdir}/vault-gc.service
%endif

%clean
rm -rf $RPM_BUILD_ROOT

%define lib_so_version 0
%files -n libvault
%defattr(-,root,root,-)
%{_libdir}/libvault-*.so.%{lib_so_version}
%{_libdir}/libvault-*.so.%{version}

%files qt5-declarative
%defattr(-,root,root,-)
%dir %{_libdir}/qt5/qml/NemoMobile/Vault
%{_libdir}/qt5/qml/NemoMobile/Vault/*

%files
%defattr(-,root,root,-)
%{_bindir}/vault
%{_bindir}/vault-sync
%{_bindir}/vault-resolve
%{_bindir}/git-remote-vault
%dir %{tools_dir}
%{tools_dir}/*
%if 0%{?_with_usersession:1}
%{_userunitdir}/vault-gc.service
%endif

%files devel
%defattr(-,root,root,-)
%{_libdir}/libvault-*.so
%{_libdir}/pkgconfig/vault-unit.pc
%dir %{_includedir}/vault
%{_includedir}/vault/*.hpp

%files tests
%defattr(-,root,root,-)
%dir /opt/tests/vault
/opt/tests/vault/*

%post -n libvault -p /sbin/ldconfig
%postun -n libvault -p /sbin/ldconfig

%post
%if 0%{?_with_usersession:1}
    systemctl-user daemon-reload || :
%endif

%postun
%if 0%{?_with_usersession:1}
    systemctl-user daemon-reload || :
%endif
