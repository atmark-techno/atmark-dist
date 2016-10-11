#!/bin/sh

ACTION=$1
DEVICE=$2

RUNPREFIX=/var/run/mjpg-streamer
RUNFILE=${RUNPREFIX}.${DEVICE}.pid

LIBDIR=/usr/lib/mjpg_streamer

make_run_file() {
    echo $1 > $RUNFILE
}

start_action() {
    if [ ! -z "`ls ${RUNPREFIX}.* 2>/dev/null`" ]; then
	return
    fi
    ledctrl red blink_on 500
    mjpg_streamer -i "${LIBDIR}/input_uvc.so --device /dev/${DEVICE} --yuv --resolution QVGA --fps 10" -o "${LIBDIR}/output_http.so --www ${LIBDIR}/www" >/dev/null 2>&1 &
    make_run_file $!
}

stop_action() {
    if [ ! -e ${RUNFILE} ]; then
	return
    fi
    kill `cat ${RUNFILE} 2>&1` >/dev/null 2>&1
    ledctrl red blink_off
    rm -f $RUNFILE
}

case $ACTION in
    start)
	start_action
	;;
    stop)
	stop_action
	;;
    *)
	;;
esac
