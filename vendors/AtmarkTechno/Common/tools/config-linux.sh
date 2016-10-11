#!/bin/sh
# FILE   : config-linux.sh 
# VER    : v1.01  
# UPDATE : 2005.9.14
#
#  Create config.linux-x.x.x
#

usage(){
    printf "need \$1 : ROOTDIR , \$2 : LINUXDIR , \$3 : VENDORDIR\n"
    printf "optional \$4 : CROSSDEV , \$5 : ARCH"
}

###############################################################################
ROOTDIR=$1
LINUXDIR=$2
VENDORDIR=$3
CROSSDEV=$4
ARCH=$5

if [ $# -lt 3 ] ; then
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

if [ "$LINUXDIR" = "linux-3.x" ] ; then
    DEFCONFIG=$DEFCONFIG_3
elif [ "$LINUXDIR" = "linux-2.6.x" ] ; then
    DEFCONFIG=$DEFCONFIG_2_6
else
    DEFCONFIG=$DEFCONFIG_2_4
fi

if [ ! $DEFCONFIG ] ; then
    printf "%s is incompatible with %s.\n" $PRODUCT $LINUXDIR
    exit 1
fi

if [ ! -f $ROOTDIR/$LINUXDIR/$DEFCONFIG ] ; then
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
DEFCONFIG_BASE=`basename $DEFCONFIG`
make ARCH=$ARCH -C $ROOTDIR/$LINUXDIR $DEFCONFIG_BASE
cp $ROOTDIR/$LINUXDIR/.config $VENDORDIR/config.$LINUXDIR
if [ "$CROSSDEV" = "armel" ]; then
    sed -i "s/# CONFIG_AEABI is not set/CONFIG_AEABI=y/" $VENDORDIR/config.$LINUXDIR
fi
if [ "$CROSSDEV" = "arm" ]; then
    sed -i "s/CONFIG_AEABI=y/# CONFIG_AEABI is not set/" $VENDORDIR/config.$LINUXDIR
fi
printf "\n#\n# Kernel data\n#\n" >> $VENDORDIR/config.$LINUXDIR
printf "KERNEL_VERSION=%s\n" $KERNEL_RELEASE >> $VENDORDIR/config.$LINUXDIR

exit 0
