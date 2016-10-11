#!/bin/sh
# FILE   : auto-strip.sh
# VER    : v1.00
# UPDATE : 2006.5.25
#
# Auto stripper
#
###############################################################################
VER="v1.00"
DEBUG="off"
LANG=C

CROSS_STRIP="$CROSS_COMPILE"strip
CROSS_OBJDUMP="$CROSS_COMPILE"objdump

###############################################################################
debug(){
    test ! "$DEBUG" = "on" && return 1
    fmt=$1
    arg=
    shift

    while test ! -z "$1"
    do
	arg="$arg $1"
	shift
    done
    printf "$fmt" $arg
}

check_root_user(){
    user=`whoami`
    debug "user: %s\n" $user
    if [ "$user" = "root" ] ; then
	return 1
    fi
    return 0
}

###############################################################################
is_script(){
    if [ "`file $1 | grep "shell script"`" ] ; then
	return 1
    fi
    return 0
}

is_cross_object(){
    $CROSS_OBJDUMP -f $1 2>/dev/null | \
	grep -e "architecture:" | \
	grep -e "$ARCH" > /dev/null 2>&1 ;

    if [ $? = 0 ] ; then
	return 1
    fi
    return 0
}

is_shared_object(){
    if [ "`file $1 | grep "shared object"`" ] ; then
	return 1
    fi
    return 0
}

strip_object(){
    is_shared_object $1
    if [ $? = 1 ] ; then
	#Shared Library
	$CROSS_STRIP --remove-section=.comment --remove-section=.note \
		     --strip-unneeded $1 > /dev/null
    else
	#Executable Binary
	$CROSS_STRIP --remove-section=.comment --remove-section=.note $1 \
		     > /dev/null
    fi
    printf "stripped: %s\n" $1
}

###############################################################################
check_regular_file(){
    for i in `find $ROMFSDIR -type f | grep -v -e "\.ko" -e "\.o"`
    do
	is_script $i
	if [ $? = 1 ] ; then continue ; fi
	
	is_cross_object $i
	if [ $? != 1 ] ; then continue ; fi

	strip_object $i
    done
}

###############################################################################
# main()
#

check_root_user
if [ $? = 1 ] ; then
    printf "Only execute this script as a normal user (not root).\n"
    exit 1
fi

printf "Auto stripper %s\n" $VER

if [ $1 ] ; then
    ROMFSDIR=$1
else
    ROMFSDIR=romfs
fi

if [ ! -e "$ROMFSDIR" ] && [ ! -d "$ROMFSDIR" ] ; then
    printf "%s is not directory.\n" $ROMFSDIR
    exit 1
fi

debug "ROMFSDIR: %s\n" `(cd $ROMFSDIR; pwd)`
debug "CROSS_STRIP: %s\n" $CROSS_STRIP

check_regular_file

exit 0
