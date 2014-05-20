
Summary: Set of tools to perform backup/restoration of the data.
Name: vault
Version: 0.1.0
Release: 1
License: LGPL21
Group: Development/Liraries
URL: https://git.jollamobile.com/vault/vault
Source0: %{name}-%{version}.tar.bz2
BuildRequires: cmake >= 2.8
BuildRequires: cor-devel
BuildRequires: gittin-devel
BuildRequires: tut
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
Set of tools to perform backup/restoration of the data.

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
%{_libdir}/libcutespp.so*
%{_libdir}/libvault-core.so*
%{_libdir}/libvault-transfer.so*
%{_libdir}/libvault-unit.so*
%{_libdir}/qt5/qml/Vault/*
%{_bindir}/vault-cli

%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/*.pc
%dir %{_includedir}/vault
%{_includedir}/vault/*.hpp


%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig
