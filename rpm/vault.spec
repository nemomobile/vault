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
%{_libdir}/libqtaround.so*
%{_libdir}/libvault-core.so*
%{_libdir}/libvault-transfer.so*
%{_libdir}/libvault-unit.so*
%{_libdir}/qt5/qml/NemoMobile/Vault/*
%{_bindir}/vault

%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/*.pc
%dir %{_includedir}/vault
%dir %{_includedir}/qtaround
%{_includedir}/vault/*.hpp
%{_includedir}/qtaround/*.hpp


%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig
