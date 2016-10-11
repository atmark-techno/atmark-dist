#!/bin/sh

ACTION=$1
DEVICE=$2

RUNPREFIX=/var/run/mjpg-streamer
RUNFILE=${RUNPREFIX}.${DEVICE}.pid

LIBDIR=/usr/lib/mjpg_streamer

OPT_RES=VGA
OPT_FPS=15

make_run_file() {
    echo $1 > $RUNFILE
}

start_action() {
    if [ ! -z "`ls ${RUNPREFIX}.* 2>/dev/null`" ]; then
	return
    fi
    mjpg_streamer -i "${LIBDIR}/input_uvc.so --device /dev/${DEVICE} \
                      --yuv --resolution ${OPT_RES} --fps ${OPT_FPS}" \
                  -o "${LIBDIR}/output_http.so --www ${LIBDIR}/www" \
        >/dev/null 2>&1 &
    make_run_file $!
}

stop_action() {
    if [ ! -e ${RUNFILE} ]; then
	return
    fi
    kill `cat ${RUNFILE} 2>&1` >/dev/null 2>&1
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
