#!/bin/sh
# FILE   : lib-inst.sh
# VER    : v2.00
# UPDATE : 2006.5.22
#
# Shard library installer
#
###############################################################################
VER="v2.00"
DEBUG="off"
LANG=C

CROSS_GCC="$CROSS_COMPILE"gcc
CROSS_OBJDUMP="$CROSS_COMPILE"objdump

GCC_VERSION=`$CROSS_GCC -dumpversion`
CROSS_LIB_DIR=
for dir in `$CROSS_GCC -print-search-dirs | \
	    grep -e "libraries: =" | \
	    sed -e "s/libraries: =//" -e "s/:/ /g"`
do
    CROSS_LIB_DIR=`(cd $dir 2>/dev/null;pwd) | \
		   grep -v "$GCC_VERSION" 2>/dev/null`
done

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

###############################################################################
is_symlink(){
    if [ "`file $1 | grep "symbolic link"`" ] ; then
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

get_link(){
    is_symlink $1
    if [ ! $? ] ; then return ; fi
    file $1 | sed -e "s/'//"  -e "s/[^\`]*//" -e "s/\`//" | 
    while read sl 
    do
	echo $sl
    done
}

cp_library(){
    TARGET_PREFIX=`echo $1 | sed -e "s/[^\/]*$//"`
    TMP_WORD=`echo $TARGET_PREFIX | sed -e "s/\///g"`
    TARGET=`echo $1 | sed -e "s/\///g" -e "s/$TMP_WORD//"`

    while true
    do
	is_symlink "$TARGET_PREFIX""$TARGET"
	if [ $? = 1 ] ; then
	    NEXT=`get_link "$TARGET_PREFIX"$TARGET`
	    echo "ln -sf /lib/$NEXT $ROMFSDIR/lib/$TARGET"
	    ln -sf /lib/$NEXT $ROMFSDIR/lib/$TARGET
	    ln -sf /lib/$NEXT $ROMFSDIR/usr/lib/$TARGET
	    TARGET="$NEXT"
	else
	    if [ ! -f "$TARGET_PREFIX""$TARGET" ] ; then
		debug "warning: not found %s\n" "$TARGET_PREFIX""$TARGET"
		break
	    fi
	    echo "cp -f "$TARGET_PREFIX""$TARGET" $ROMFSDIR/lib"/$TARGET
	    cp -f "$TARGET_PREFIX""$TARGET" $ROMFSDIR/lib/$TARGET
	    check_library $ROMFSDIR/lib/"$TARGET"
	    break
	fi
    done
}

check_library(){
    $CROSS_OBJDUMP -p $1 | grep NEEDED | \
	sed -e "s/NEEDED//" |
	while read i 
	do 
	    if [ ! -f "$ROMFSDIR"/lib/$i ] ; then
		if [ ! -L "$ROMFSDIR"/lib/$i ]; then
		    cp_library "$CROSS_LIB_DIR"/$i
		fi
	    fi
	done
}

###############################################################################
check_regular_file(){
    for i in `find $ROMFSDIR -type f`
    do
	is_cross_object $i
	if [ $? = 1 ] ; then
	    check_library $i
	fi
    done
}

check_nsswitch(){
    if [ -f $ROMFSDIR/etc/nsswitch.conf ] ; then
	for i in `cat $ROMFSDIR/etc/nsswitch.conf | sed -e "s/.*://"`
	do
	    if [ ! -e "$ROMFSDIR"/lib/libnss_$i.so ] ; then
		cp_library "$CROSS_LIB_DIR"/libnss_$i.so
	    fi
	done
    fi
}

###############################################################################
# main()
#
printf "Shard-Library installer %s\n" $VER

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
debug "CROSS_GCC: %s\n" $CROSS_GCC
debug "CROSS_LIB_DIR: %s\n" $CROSS_LIB_DIR

check_regular_file
check_nsswitch

exit 0
