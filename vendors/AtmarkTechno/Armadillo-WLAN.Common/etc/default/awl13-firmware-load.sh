#!/bin/sh

[ -f /etc/config/awl13.conf ] && \
. /etc/config/awl13.conf

WLAN=$1
FIRMWARE_SDIO=$(ls -1v /lib/firmware/awl13/fwimage*${AWL13_MODE}_SDIO.bin | tail -1)
FIRMWARE_USB=$(ls -1v /lib/firmware/awl13/fwimage*${AWL13_MODE}_USB.bin | tail -1)

[ -f /sys/module/awl13_sdio/$WLAN/firmware ] && \
cat $FIRMWARE_SDIO > /sys/module/awl13_sdio/$WLAN/firmware
[ -f /sys/module/awl13_usb/$WLAN/firmware ] && \
cat $FIRMWARE_USB > /sys/module/awl13_usb/$WLAN/firmware
iwpriv $WLAN fwload
iwpriv $WLAN fwsetup
