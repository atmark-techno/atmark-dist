#!/bin/sh
#
# based on udev-175/udev/udev-builtin-input_id.c
#

ACTION=$1
DEVPATH=$2

DEV=/dev/input/`basename /sys/${DEVPATH}`
IDEVPATH=/sys/`dirname ${DEVPATH}`
INPUTNUM=`basename ${IDEVPATH}`
DEVFILE=c`cat /sys/${DEVPATH}/dev`

 EV_CAPS=`cat ${IDEVPATH}/capabilities/ev`
REL_CAPS=`cat ${IDEVPATH}/capabilities/rel`
ABS_CAPS=`cat ${IDEVPATH}/capabilities/abs`
KEY_CAPS=`cat ${IDEVPATH}/capabilities/key`

### INPUT definitions from include/linux/input.h ###
EV_KEY=0x01
EV_REL=0x02
EV_ABS=0x03

BTN_1=0x101
BTN_MOUSE=0x110
BTN_LEFT=0x110
BTN_TRIGGER=0x120
BTN_A=0x130
BTN_TOOL_PEN=0x140
BTN_TOOL_FINGER=0x145
BTN_TOUCH=0x14a
BTN_STYLUS=0x14b

REL_X=0x00
REL_Y=0x01

ABS_X=0x00
ABS_Y=0x01
ABS_MT_POSITION_X=0x35
ABS_MT_POSITION_Y=0x36

### Functions ###
testbit()
{
    local bits=${1}
    local mask=${2}

    if test `echo $((${bits} & ${mask}))` -gt 0; then
	return 0
    else
	return 1
    fi
}

evtestbit_internal()
{
    local bits=${1}
    local mask=`echo $((1 << ${2}))`

    testbit ${bits} ${mask}
    return $?
}

evtestbit()
{
    local BIT=`echo $((${1}))` #hex2dec
    shift
    local BITS_ARRAY=${*}
    local INV_BITS_ARRAY

    for i in ${BITS_ARRAY}; do
	INV_BITS_ARRAY=`echo 0x$i ${INV_BITS_ARRAY}`
    done

    local OFFSET=0
    for i in ${INV_BITS_ARRAY}; do
	if test ${BIT} -gt 31; then
	    BIT=$((${BIT} - 32))
	else
	    evtestbit_internal ${i} ${BIT}
	    return $?
	fi
    done
    return 1 #FALSE
}

test_pointer()
{
    if evtestbit ${EV_ABS} ${EV_CAPS} && \
       (evtestbit ${ABS_X} ${ABS_CAPS} || \
	evtestbit ${ABS_MT_POSITION_X} ${ABS_CAPS}) && \
       (evtestbit ${ABS_Y} ${ABS_CAPS} || \
	evtestbit ${ABS_MT_POSITION_Y} ${ABS_CAPS}); then

	if evtestbit ${BTN_STYLUS} ${KEY_CAPS} || \
	   evtestbit ${BTN_TOOL_PEN} ${KEY_CAPS}; then

	    input_id="ID_INPUT_TABLET"

	elif evtestbit ${BTN_TOOL_FINGER} ${KEY_CAPS} && \
	     ! evtestbit ${BTN_TOOL_PEN} ${KEY_CAPS}; then

	    input_id="ID_INPUT_TOUCHPAD"

	elif evtestbit ${BTN_TRIGGER} ${KEY_CAPS} || \
	     evtestbit ${BTN_A} ${KEY_CAPS} || \
	     evtestbit ${BTN_1} ${KEY_CAPS}; then

	    input_id="ID_INPUT_JOYSTICK"

	elif evtestbit ${BTN_MOUSE} ${KEY_CAPS}; then

	    input_id="ID_INPUT_MOUSE"

	elif evtestbit ${BTN_TOUCH} ${KEY_CAPS}; then

	    input_id="ID_INPUT_TOUCHSCREEN"

	elif evtestbit ${ABS_MT_POSITION_X} ${ABS_CAPS} && \
	     evtestbit ${ABS_MT_POSITION_Y} ${ABS_CAPS}; then

	    input_id="ID_INPUT_TOUCHSCREEN" #Multi-touch

	fi
    fi

    if evtestbit ${EV_REL} ${EV_CAPS} && \
       evtestbit ${REL_X} ${REL_CAPS} && \
       evtestbit ${REL_Y} ${REL_CAPS} && \
       evtestbit ${BTN_MOUSE} ${KEY_CAPS}; then

	input_id="ID_INPUT_MOUSE"

    fi
}

test_key()
{
    # the first 32 bits are ESC, numbers, and Q to D; if we have all of
    # those, consider it a full keyboard; do not test KEY_RESERVED, though
    for i in ${KEY_CAPS}; do
	KEY0=$i
    done

    if test "${KEY0}" = "fffffffe"; then #test 0xfffffffe as STRINGS

	input_id="ID_INPUT_KEYBOARD"

    fi
}

create_udev_input_data()
{
    if test "${input_id}"; then
	mkdir -p /run/udev/data
	echo "E:${input_id}=1" > /run/udev/data/${DEVFILE}
	ln -s /run/udev/data/${DEVFILE} /run/udev/data/+input:${INPUTNUM}
    fi
}

remove_udev_input_data()
{
    rm -f `readlink /run/udev/data/+input:${INPUTNUM}`
    rm -f /run/udev/data/+input:${INPUTNUM}
}

case "${ACTION}" in
    add)
	test_pointer
	test_key
	create_udev_input_data
	;;
    remove)
	remove_udev_input_data
	;;
    *)
	echo "Usage: ${0} {add|remove} DEVPATH"
	exit 1
	;;
esac

exit 0
