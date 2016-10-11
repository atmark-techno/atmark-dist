%define version 0.9.0

Summary: ARMS Protocol Daemon
Name: armsd
Version: %{version}
Release: 0
Source0: armsd-%{version}.tar.gz
License: BSD
Vendor: Internet Initiative Japan Inc.
Group: System Environment/Daemons
Packager: Tomoyuki Sahara (tsahara@iij.ad.jp)
Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-root
URL: http://dev.smf.jp/
BuildRequires: make, gcc, libarms-devel >= 5.20
Requires: libarms

%description
ARMS protocol daemon.

%prep
%setup -q

%build
%configure
make clean
make

%install
%makeinstall cachedir=${RPM_BUILD_ROOT}/var/cache/armsd
rm -r ${RPM_BUILD_ROOT}/usr/share/armsd

%clean
rm -rf ${RPM_BUILD_ROOT}

%preun
rm -f /var/cache/armsd/*

%files
%defattr(600,root,root)
%config(noreplace) /etc/armsd/armsd.conf
%defattr(-,root,root)
%dir /etc/armsd/scripts
%dir /var/cache/armsd
%doc examples
%{_sbindir}/armsd


%changelog
* Wed Mar 13 2013 Akihiro Yamazaki <yamazaki@iij.ad.jp> 0.9.0-0
- catchup with libarms 5.20
- add command line options: -D, -v, -w
- obsolete command line option: -d
- change armsd.conf parameters
- fix some bugs

* Wed Aug 29 2012 Tomoyuki Sahara <tsahara@iij.ad.jp> 0.8.0-0
- "https-proxy-url-ls" and "https-proxy-url-rs"

* Mon Apr 9 2012 Tomoyuki Sahara <tsahara@iij.ad.jp> 0.7.0-0
- add "hb-disk-usage" configuration parameter
- install armsd to /usr/sbin

* Tue Mar 27 2012 Tomoyuki Sahara <tsahara@iij.ad.jp> 0.6-0
- renamed to armsd.
- initial-config is no longer needed.

* Fri Mar 23 2012 Tomoyuki Sahara <tsahara@iij.ad.jp> 0.5-3
- catch up with libarms 5.00
- change license

* Thu Sep 29 2011 Tomoyuki Sahara <tsahara@iij.ad.jp> 0.5-2
- install sample configuration files
- mkdir /var/cache/marmsd
- better Linux support: Heartbeat parameters, ping, traceroute

* Wed Feb 23 2011 Tomoyuki Sahara <tsahara@iij.ad.jp> 0.4-1
support Heartbeat.

* Mon Feb 21 2011 Tomoyuki Sahara <tsahara@iij.ad.jp> 0.3-0
initial RPM release.
