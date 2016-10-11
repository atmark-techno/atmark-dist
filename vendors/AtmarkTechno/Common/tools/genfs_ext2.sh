#!/bin/sh
# FILE   : genfs_ext2.sh
# VER    : v2.00
# UPDATE : 2010.2.26
#
#  This file is a script that solve the option of genext2fs,
#  and executes genext2fs.
#

#DEBUG=yes

GENEXT2FS=/usr/bin/genext2fs
GENEXT2FSOPT=$1

BLOCK_SIZE=1024
EXT2_FIRST_INO=11 #First non reserved inode

# multiplier of blocks and inodes
BMUL=${GENFS_EXT2_BMUL:-20}
IMUL=${GENFS_EXT2_IMUL:-20}

insufficient_option() {
    echo ""
    echo "$GENEXT2FS doesn't seems to support option '$1'"
    echo "Please install appropriate version"
    echo ""
}

check_genext2fs() {
    local TEMP=$(mktemp /tmp/_temp_$$_XXXXXX)
    # do we have it installed?
    if [ ! -x $GENEXT2FS ]; then
	echo ""
	echo "Can't find genext2fs. Please install"
	echo "    http://armadillo.atmark-techno.com/faq/genext2fs-not-found"
	echo ""
	return 1
    fi

    # get help output; this is the only way to know what we can do. sigh...
    $GENEXT2FS -h 2> $TEMP

    for opt in root squash-uids devtable size-in-blocks number-of-inodes; do
	grep -- " --$opt" $TEMP 2>&1 > /dev/null
	if [ $? -ne 0 ]; then
	    insufficient_option $opt
	    rm -f $TEMP
	    exit 1
	fi
    done
    rm -f $TEMP
    return 0
}

check_genext2fs

#
# calc nr_blocks
#
NEEDBLOCKS=0
for i in `find ${ROMFSDIR} -type f`
do
  set `wc -c $i`
  NEEDBLOCKS=$(($NEEDBLOCKS+($1+$BLOCK_SIZE-1)/BLOCK_SIZE))
done

#
# calc nr_inodes
#
nr_files=$(find ${ROMFSDIR} -follow | wc -l)
nr_devnode=$(awk '/^\// { inode += $10 == "-" ? 1 : $10 } END { print inode }' ext2_devtable.txt)
NEEDINODES=$(($EXT2_FIRST_INO+$nr_files+$nr_devnode))

if [ x$DEBUG = xyes ] ; then
    printf "\tblocks\tinodes\n"
    printf "need\t%s\t%s\n" $NEEDBLOCKS $NEEDINODES
fi

OPTBLOCKS=$(($NEEDBLOCKS+$NEEDBLOCKS/100*$BMUL))
OPTINODES=$(($NEEDINODES+$NEEDINODES/100*$IMUL))

cmd="$GENEXT2FS --size-in-blocks $OPTBLOCKS --number-of-inodes $OPTINODES $GENEXT2FSOPT"
echo $cmd
$cmd
