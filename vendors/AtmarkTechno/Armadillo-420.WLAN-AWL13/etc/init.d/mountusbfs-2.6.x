#!/bin/sh

. /etc/init.d/functions

PATH=/bin:/sbin:/usr/bin:/usr/sbin

echo -n "Mounting usbfs: "
mount -t usbfs usbfs /proc/bus/usb
check_status
