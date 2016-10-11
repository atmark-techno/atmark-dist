#!/bin/sh
# FILE   : config-linux.sh 
# VER    : v1.01  
# UPDATE : 2005.9.14
#
#  Create config.linux-x.x.x
#

usage(){
    printf "need \$1 : ROOTDIR , \$2 : LINUXDIR , \$3 : VENDORDIR\n"
}

###############################################################################
ROOTDIR=$1
LINUXDIR=$2
VENDORDIR=$3

if [ $# != 3 ] ; then
    usage
    exit 1
fi

if [ ! -d $ROOTDIR/$LINUXDIR ] ; then
    printf "%s is not directory.\n" $ROOTDIR/$LINUXDIR
    exit 1
fi

if [ ! -f $VENDORDIR/tools/config-linux.conf ] ; then
    printf "%s is not found.\n" $VENDORDIR/tools/config-linux.conf
    exit 1
else
    . $VENDORDIR/tools/config-linux.conf
fi

if [ "$LINUXDIR" = "linux-2.6.x" ] ; then
    DEFCONFIG=$DEFCONFIG_2_6
else
    DEFCONFIG=$DEFCONFIG_2_4
fi

if [ ! -e $ROOTDIR/$LINUXDIR/$DEFCONFIG ] ; then
    printf "%s is not found.\n" $ROOTDIR/$LINUXDIR/$DEFCONFIG
    exit 1
fi

###############################################################################
K_VERSION=`grep "^VERSION" $ROOTDIR/$LINUXDIR"/Makefile" | \
                        head -1 | sed -e "s/VERSION = //" `
K_PATCHLEVEL=`grep "^PATCHLEVEL" $ROOTDIR/$LINUXDIR"/Makefile" | \
                        head -1 | sed -e "s/PATCHLEVEL = //" `
K_SUBLEVEL=`grep "^SUBLEVEL" $ROOTDIR/$LINUXDIR"/Makefile" | \
                        head -1 | sed -e "s/SUBLEVEL = //" `
K_EXTRAVERSION=`grep "^EXTRAVERSION" $ROOTDIR/$LINUXDIR"/Makefile" | \
                        head -1 | sed -e "s/EXTRAVERSION = //" `
KERNEL_RELEASE=$K_VERSION"."$K_PATCHLEVEL"."$K_SUBLEVEL$K_EXTRAVERSION

###############################################################################
#echo cp $ROOTDIR/$LINUXDIR/$DEFCONFIG config.$LINUXDIR
rm -f $VENDORDIR/config.$LINUXDIR
cp $ROOTDIR/$LINUXDIR/$DEFCONFIG $VENDORDIR/config.$LINUXDIR
printf "\n#\n# Kernel data\n#\n" >> $VENDORDIR/config.$LINUXDIR
printf "KERNEL_VERSION=%s\n" $KERNEL_RELEASE >> $VENDORDIR/config.$LINUXDIR

exit 0
