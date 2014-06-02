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
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5Qml)
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
# last known available version was 0.8.17.1
Provides: the-vault = 0.8.19
Obsoletes: the-vault <= 0.8.18

%description
Incremental backup/restore framework

%package devel
Summary: vault headers etc.
Group: System Environment/Libraries
Requires: %{name} = %{version}-%{release}
%description devel
vault library header files etc.

%package -n qtaround
Summary: QtAround library
Group: System Environment/Libraries
%description -n qtaround
QtAround library used to port the-vault to C++. Mostly consists of
thin wrappers around Qt classes and standard Linux utilities.

%package -n qtaround-devel
Summary: QtAround library
Group: System Environment/Libraries
Requires: qtaround = %{version}-%{release}
%description -n qtaround-devel
QtAround library used to port the-vault to C++. Mostly consists of
thin wrappers around Qt classes and standard Linux utilities.

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

%files -n qtaround
%defattr(-,root,root,-)
%{_libdir}/libqtaround.so*

%files -n qtaround-devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/qtaround.pc
%dir %{_includedir}/qtaround
%{_includedir}/qtaround/*.hpp

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%post -n qtaround -p /sbin/ldconfig
%postun -n qtaround -p /sbin/ldconfig
