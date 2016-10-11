#!/bin/sh

SERIAL_3G_LOCK="/var/lock/serial-3g.lock"
PID_3G=1

support_3g_hl8548_check() {
    flock $SERIAL_3G_LOCK expect /usr/share/3g/3g-hl8548-support-check.exp
}

restore_stty_setting_handler() {
	local SPEED=$(stty -F /dev/ttyATCMD speed)
	local PAST=$(stty -F /dev/ttyATCMD -g)
	trap "stty -F /dev/ttyATCMD $PAST $SPEED" TERM EXIT QUIT
}
