#!/bin/sh

WLAN=$1
FIRMWARE=/lib/firmware/aerial/fwimage221r_STA_SDIO.bin

cat $FIRMWARE > /sys/module/aerial/$WLAN/firmware
iwpriv $WLAN fwload
