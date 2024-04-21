%define mu_path %{getenv:MU_MODULE_ROOT}
%define _topdir %{mu_path}/rpmbuild
%define buildroot %{_topdir}/%{name}-%{version}-%{release}.%{buildarch}
%define binpath %{buildroot}/endurox-muserver
%define packagername %(who am i | awk '{print $1}')

BuildRoot:	%{buildroot}
Name:		EnduroX-MUserver
Version:	%{getenv:MU_VERSION}
Release:	%{getenv:GIT_VERSION}%{?dist}
Summary:	EnduroX MuOnline Server
License:	R.G.S.

Requires: endurox

%description
EnduroX MuOnline Server

%pre
if [ `grep -c ^muonline /etc/passwd` = "0" ]; then
/usr/sbin/useradd -c 'MuOnline user' -d /home/muonline -s /bin/bash muonline
fi

%install
mkdir -p %{binpath}/bin
mkdir -p %{binpath}/ubftab
mkdir -p %{binpath}/conf
mkdir -p %{binpath}/log
mkdir -p %{binpath}/lib
mkdir -p %{binpath}/protocol
mkdir -p %{binpath}/settings
mkdir -p %{binpath}/terrain

cp %{getenv:MU_MODULE_ROOT}/dist/bin/* %{binpath}/bin
cp %{getenv:MU_MODULE_ROOT}/dist/ubftab/* %{binpath}/ubftab
cp %{getenv:MU_MODULE_ROOT}/protocol/Dec1.dat %{binpath}/protocol/Dec1.dat
cp %{getenv:MU_MODULE_ROOT}/protocol/Enc2.dat %{binpath}/protocol/Enc2.dat
cp %{getenv:MU_MODULE_ROOT}/config/muonline/settings/* %{binpath}/settings
cp %{getenv:MU_MODULE_ROOT}/config/muonline/terrain/* %{binpath}/terrain
cp %{getenv:MU_MODULE_ROOT}/config/endurox/* %{binpath}/conf

%post
chown -R muonline:muonline /endurox-muserver
mkdir -p /endurox-muserver/log

%files
%defattr(-,muonline,muonline)
%dir /endurox-muserver
/endurox-muserver/bin/*
%config(noreplace) /endurox-muserver/config/*
%config(noreplace) /endurox-muserver/settings/*
/endurox-muserver/ubftab/*
/endurox-muserver/terrain/*
/endurox-muserver/protocol/*