/*
 * at-cgi-core/at-cgi-core.h
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#ifndef ATCGICORE_H_
#define ATCGICORE_H_

#define ATCGI_VERSION		"1.64"

#define TITLE_IMG			"/images/at-admin.jpg"
#define SOFTWARE_NAME		"AT Admin"
#define HEAD_TITLE			"AT Admin"

#define MAX_QUERY_DATA_SIZE	(32768)

#define PRODUCT_NAME_A220		"Armadillo-220"
#define PRODUCT_NAME_A230		"Armadillo-230"
#define PRODUCT_NAME_A240		"Armadillo-240"
#define PRODUCT_NAME_A9			"Armadillo-9"

#define CONFIG_PATH			"/etc/config"
#define FLATFSD_CONFIG_PATH	CONFIG_PATH
#define HOSTNAME_FILE_PATH	CONFIG_PATH "/HOSTNAME"
#define INTERFACES_PATH		CONFIG_PATH "/interfaces"
#define BRCONFIG_PATH		CONFIG_PATH "/bridges"
#define RESOLV_CONF_PATH	CONFIG_PATH "/resolv.conf"
#define HOSTS_CONF_PATH		CONFIG_PATH "/hosts"
#define CRONTAB_CONFIG_PATH	CONFIG_PATH "/root.crontab"
#define DISTNAME_PATH		"/etc/DISTNAME"
#define HTPASSWD_FILE_NAME	"/home/www-data/admin/.htpasswd"

#define MEMINFO_PATH		"/proc/meminfo"
#define UPTIME_PATH			"/proc/uptime"
#define LOADAVG_PATH		"/proc/loadavg"

#define AVAHI_INIT_PATH		"/etc/init.d/avahi-daemon"
#define BRIDGE_INIT_PATH	"/etc/init.d/bridges"
#define SNORT_INIT_PATH		"/etc/init.d/snort"

#define SYSLOG_PATH			"/var/log/messages"

#define AVAHI_PATH			"/sbin/avahi-daemon"
#define IFUP_PATH			"/sbin/ifup"
#define IFDOWN_PATH			"/sbin/ifdown"
#define REBOOT_PATH			"/sbin/reboot"
#define IFCONFIG_PATH		"/sbin/ifconfig"
#define PIDOF_PATH			"/bin/pidof"
#define HOSTNAME_PATH		"/bin/hostname"
#define NETFLASH_PATH		"/bin/netflash"
#define TOUCH_PATH			"/bin/touch"
#define BRCTL_PATH			"/bin/brctl"
#define FLATFSD_PATH		"/bin/flatfsd"
#define WGET_PATH			"wget"
#define SUDO_PATH			"/usr/bin/sudo"
#define UNAME_PATH			"/bin/uname"
#define CAT_PATH			"/bin/cat"
#define RM_PATH				"/bin/rm"
#define CP_PATH				"/bin/cp"
#define LED_CTRL_PATH		"/bin/ledctrl"
#define CRONTAB_PATH		"/usr/bin/crontab"
#define MAIL_PATH			"/bin/mail"

#define USERLAND_FLASH_PATH		"/dev/flash/userland"
#define KERNEL_FLASH_PATH		"/dev/flash/kernel"
#define NETFLASH_RESULT_PATH	"/tmp/at-admin/netflash-result"

#endif /*ATCGICORE_H_*/
