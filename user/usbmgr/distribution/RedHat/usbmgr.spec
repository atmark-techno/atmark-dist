%define	name	usbmgr
%define	version	1.0.0
%define	release	1

Summary:	usbmgr - usbmgr is user-mode daemon which loads and unloads USB kernel modules
Name:		%{name}
Version:	%{version}
Release:	%{release}
Copyright:	GPL
Group:		System Environment
URL:		http://www.dotAster.com/~shuu/linux/usbmgr/
Vendor:		Shuu Yamaguchi <shuu@dotAster.com>
Source:		%{name}-%{version}.tar.gz
BuildRoot:	/var/tmp/%{name}-%{version}

%description
usbmgr is user-mode daemon which loads and unloads USB kernel modules according to the configuration.

%prep
%setup -q

%build
./configure --sbindir=/sbin --sysconfdir=/etc
make

%install
[ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT;
mkdir -p $RPM_BUILD_ROOT
[ -d $RPM_BUILD_ROOT/sbin ] || mkdir $RPM_BUILD_ROOT/sbin;
[ -d $RPM_BUILD_ROOT/etc/rc.d/init.d ] || mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d;
[ -d $RPM_BUILD_ROOT/etc/rc.d/rc5.d ] || mkdir $RPM_BUILD_ROOT/etc/rc.d/rc5.d;
make BIN_DIR=$RPM_BUILD_ROOT/sbin CONF_DIR=$RPM_BUILD_ROOT/etc/usbmgr RC_INIT_DIR=$RPM_BUILD_ROOT/etc/rc.d/init.d install use_mouse

%clean
[ -d $RPM_BUILD_ROOT ] && rm -rf $RPM_BUILD_ROOT;

%files
%defattr(-,root,root)
%doc CHANGES README README.eucJP TODO
/sbin/usbmgr
/sbin/update_usbdb
/sbin/dump_usbdev
/etc/rc.d/init.d/usbmgr
/etc/rc.d/rc5.d/S46usbmgr
%config /etc/usbmgr/preload.conf
%config /etc/usbmgr/usbmgr.conf
%config /etc/usbmgr/network

%changelog

