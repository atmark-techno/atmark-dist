#!/bin/sh
# FILE   : lib-inst.sh
# VER    : v2.01
# UPDATE : 2007.9.26
#
# Shard library installer
#
###############################################################################
VER="v2.01"
DEBUG="off"
LANG=C

CROSS_GCC="$CROSS_COMPILE"gcc
CROSS_OBJDUMP="$CROSS_COMPILE"objdump
TRIPLET=`$CROSS_GCC -print-multiarch 2>/dev/null`

GCC_VERSION=`$CROSS_GCC -dumpversion`
CROSS_LIB_DIR=
for dir in `$CROSS_GCC -print-search-dirs | \
	    grep -e "libraries: =" | \
	    sed -e "s/libraries: =//" -e "s/:/ /g"`
do
    if [ ! -d $dir ]; then
        continue
    fi
    CROSS_LIB_DIR=`(cd $dir 2>/dev/null;pwd) | \
		   grep -v "$GCC_VERSION" 2>/dev/null`
    if [ "$CROSS_LIB_DIR" ]; then
        dir_is_found=n
        for lib_dir in $CROSS_LIB_DIRS; do
            if [ "$CROSS_LIB_DIR" = "$lib_dir" ]; then
                dir_is_found=y
            fi
        done
        if [ "$dir_is_found" = "n" ]; then
            CROSS_LIB_DIRS="$CROSS_LIB_DIRS $CROSS_LIB_DIR"
        fi
    fi
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
	grep -e "$TARGET_ARCH" > /dev/null 2>&1 ;

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
    TARGET_PREFIX=`dirname $1`"/"
    TARGET=`basename $1`

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

is_relative_path(){
    if [ "`echo $1 | cut -c1`" != "/" ]; then
	return 1;
    fi
    return 0
}

get_rpath_dir(){
    case $1 in
	"/usr/lib/$TRIPLET/"*)
	    BASE="/usr/lib/$TRIPLET/";;
	"/lib/$TRIPLET/"*)
	    BASE="/lib/$TRIPLET/";;
	"/usr/lib/"*)
	    BASE="/usr/lib/";;
	"/lib/"*)
	    BASE="/lib/";;
	*)
	    return;;
    esac

    echo ${1##"$BASE"}
}

cp_library_r(){
    CROSS_LIB_PREFIX=/usr/$TRIPLET/lib
    TARGET_PREFIX=`dirname $1`
    TARGET=`basename $1`

    while true
    do
	RPATH_DIR=`get_rpath_dir $TARGET_PREFIX`
	if [ -z "$RPATH_DIR" ]; then
	    return 1
	fi
	CROSS_TARGET_PREFIX=$CROSS_LIB_PREFIX/$RPATH_DIR/
	if [ ! "$(ls $CROSS_TARGET_PREFIX/$TARGET 2> /dev/null)" ]; then
	    return 1
	fi

	mkdir -p $ROMFSDIR/$TARGET_PREFIX
	is_symlink "$CROSS_TARGET_PREFIX/$TARGET"
	if [ $? = 1 ] ; then
	    NEXT=`get_link $CROSS_TARGET_PREFIX/$TARGET`
	    echo "ln -sf $NEXT $ROMFSDIR/$TARGET_PREFIX/$TARGET"
	    ln -sf $NEXT $ROMFSDIR/$TARGET_PREFIX/$TARGET

	    is_relative_path $NEXT
	    if [ $? = 1 ]; then
		NEXT="$TARGET_PREFIX/$NEXT"
	    fi
	    TARGET="$NEXT"
	else
	    if [ ! -f "$CROSS_TARGET_PREFIX/$TARGET" ] ; then
		debug "warning: not found %s\n $CROSS_TARGET_PREFIX/$TARGET"
		break
	    fi
	    echo "cp -f $CROSS_TARGET_PREFIX/$TARGET $ROMFSDIR/$TARGET_PREFIX/$TARGET"
	    cp -f $CROSS_TARGET_PREFIX/$TARGET $ROMFSDIR/$TARGET_PREFIX/$TARGET
	    check_library $ROMFSDIR/$TARGET_PREFIX/$TARGET
	    return 0
	fi
    done

    return 1
}

get_rpaths(){
    echo `$CROSS_OBJDUMP -p $1 | grep RPATH | \
	  sed -e "s/RPATH//" | sed -e "s/:/ /g"`
}

check_library(){
    RPATHS=`get_rpaths $1`
    $CROSS_OBJDUMP -p $1 | grep NEEDED | \
	sed -e "s/NEEDED//" |
	while read i 
	do 
	    for rpath in $RPATHS; do
		if [ ! "$(ls $ROMFSDIR/$rpath/$i 2> /dev/null)" ]; then
		    cp_library_r $rpath/$i
		    if [ $? = 0 ]; then
			continue
		    fi
		fi
	    done
	    if [ ! "$(ls $ROMFSDIR/lib/$i 2> /dev/null)" ]; then
		for lib_dir in $CROSS_LIB_DIRS; do
		    lib_is_found=n
		    for sub_dir in $LIBINST_SUBDIRS; do
			if [ "$(ls $lib_dir/$sub_dir/$i 2> /dev/null)" ]; then
			    cp_library $lib_dir/$sub_dir/$i
			    lib_is_found=y
			    break
			fi
		    done
		    if [ "$lib_is_found" = "y" ]; then
			break
		    fi
		    if [ "$(ls $lib_dir/$i 2> /dev/null)" ]; then
			cp_library $lib_dir/$i
			break
		    fi
		done
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
	    if [ ! -e "$ROMFSDIR"/lib/libnss_$i.so.[0-9] ] ; then
		cp_library "$CROSS_LIB_DIR"/libnss_$i.so.[0-9]
	    fi
	done
    fi
}

check_dynamic_linker(){
    if [ "$TRIPLET" = "arm-linux-gnueabihf" ]; then
	mkdir -p $ROMFSDIR/lib/$TRIPLET
	for ld in `ls $ROMFSDIR/lib/ld-linux*.so.*`;
	do
	    SRC=`basename $ld`
	    DST="ld-linux.so.${SRC##*.so.}"
	    echo "ln -sf /lib/$SRC $ROMFSDIR/lib/$TRIPLET/$DST"
	    ln -sf /lib/$SRC $ROMFSDIR/lib/$TRIPLET/$DST
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
if [ $ARCH = "ppc" ] ; then
    TARGET_ARCH="powerpc"
else
    TARGET_ARCH=$ARCH
fi

if [ ! -e "$ROMFSDIR" ] && [ ! -d "$ROMFSDIR" ] ; then
    printf "%s is not directory.\n" $ROMFSDIR
    exit 1
fi

debug "ROMFSDIR: %s\n" `(cd $ROMFSDIR; pwd)`
debug "CROSS_GCC: %s\n" $CROSS_GCC
debug "CROSS_LIB_DIR: %s\n" $CROSS_LIB_DIR

check_regular_file
check_dynamic_linker
check_nsswitch

exit 0
