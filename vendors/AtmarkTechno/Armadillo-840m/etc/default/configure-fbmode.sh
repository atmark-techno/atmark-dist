#!/bin/sh

ACTION=$1
DEVICE=$2
PARAM=$3

MUST_VMODE_CHANGE=y
IGNORE_MODE_1='1920x1080p-60'
IGNORE_MODE_2='0x0-0'

fbmode_reconfigure() {
    # p_: path
    # s_: strings
    local p_fbdir=/sys/class/graphics/${DEVICE}/
    local p_mode=${p_fbdir}/mode
    local p_modelist=${p_fbdir}/modes
    local p_cur_mode=/var/run/${DEVICE}.mode
    local s_cur_mode=$(cat ${p_cur_mode})

    if [ ! "x${MUST_VMODE_CHANGE}x" = "xyx" ]; then
	if [ ! -z "${s_cur_mode}" ]; then
	    grep ${s_cur_mode} ${p_modelist} > /dev/null
	    [ $? -eq 0 ] && exit
	fi
    fi

    local max_resolution=0
    local max_rate=0
    local max_mode=""

    local s_modelist="$(cat ${p_modelist} | grep -v ${IGNORE_MODE_1} \
					  | grep -v ${IGNORE_MODE_2})"
    for mode in ${s_modelist}
    do
	local width=$(echo ${mode} | sed -e "s/.*://" -e "s/x.*//")
	local height=$(echo ${mode} | sed -e "s/.*x//" -e "s/[ip].*//")
	local s_irace=$(echo ${mode} | sed -e "s/.*x[0-9]*//" -e "s/-.*//")
	local refresh=$(echo ${mode} | sed -e "s/.*-//")
	local resolution=$((${width} * ${height}))
	local mult=1
	if [ ${s_irace} = "p" ]; then
	    mult=2
	fi
	local rate=$((${resolution} * ${mult} * ${refresh}))

	if [ ${max_resolution} -lt ${resolution} ]; then
	    max_resolution=${resolution}
	    max_rate=${rate}
	    max_mode=${mode}
	elif [ ${max_resolution} -eq ${resolution} ]; then
	    if [ ${max_rate} -lt ${rate} ]; then
		max_resolution=${resolution}
		max_rate=${rate}
		max_mode=${mode}
	    fi
	fi
    done

    # save new video mode
    echo ${max_mode} > ${p_cur_mode}
    # set new video mode
    echo ${max_mode} > ${p_mode}
}

add_handler() {
    fbmode_reconfigure
}

change_hotplug_handler() {
    case "${HOTPLUG}" in
	1) # PLUGGED IN
	    ;;
	0) # UNPLUGGED
	    ;;
	*)
	    exit 1
	    ;;
    esac
}

change_modelist_handler() {
    fbmode_reconfigure
}

change_handler() {
    case "${PARAM}" in
	HOTPLUG*)
	    change_hotplug_handler
	    ;;
	MODELIST*)
	    change_modelist_handler
	    ;;
	*)
	    exit 1
	    ;;
    esac
}

case "${ACTION}" in
    add)
	add_handler
	;;
    change)
	change_handler
	;;
    *)
	echo "Usage: $0 {add|change} dev [options]"
	exit 1
	;;
esac

exit 0
