#!/bin/sh
# FILE   : genfs_ext2.sh
# VER    : v1.01
# UPDATE : 2005.7.29
#
#  This file is a script that solve the option of genext2fs,
#  and executes genext2fs.
#

is_digit(){
    if [ ! $1 ] ; then
	return 0
    fi
    BUF=`echo $1 | sed -e "s/[0-9]//g"`
    if [ ! $BUF ] ;then
	return 1
    fi
    return 0

}

TEMP=_temp_$$

genext2fs $1 >/dev/null 2>$TEMP

for i in `cat $TEMP | grep blocks | head -1`
do
    is_digit $i
    if [ $? = 1 ] ; then
	NEEDBLOCKS=$i
	break
    fi
done

for i in `cat $TEMP | grep inodes | head -1`
do
    is_digit $i
    if [ $? = 1 ] ; then
	NEEDINODES=$i
	break
    fi
done

rm -f $TEMP


#NEEDBLOCKS=`genext2fs -v $1 2>/dev/null | grep blocks | head -1 | sed "s/ .*//"`
#NEEDINODES=`genext2fs -v $1 2>/dev/null | grep inodes | head -1 | sed "s/ .*//"`
#echo "need blocks : $NEEDBLOCKS"
#echo "need inodes : $NEEDINODES"

OPTBLOCKS=$(($NEEDBLOCKS+$(($(($NEEDBLOCKS/10))*2))))
OPTINODES=$(($NEEDINODES+$(($(($NEEDINODES/10))*2))))

BLOCKS_LIST="6592 8196 12288"
INODES_LIST="1024 1536 2048"

for i in $BLOCKS_LIST
do
    if [ $OPTBLOCKS -le $i ] ; then
	OPTBLOCKS=$i
	break
    fi
done

for i in $INODES_LIST
do
    if [ $OPTINODES -le $i ] ; then
	OPTINODES=$i
	break
    fi
done


#echo "opt blocks : $OPTBLOCKS"
#echo "opt inodes : $OPTINODES"

echo "genext2fs -b $OPTBLOCKS -i $OPTINODES $1"
genext2fs -b $OPTBLOCKS -i $OPTINODES $1 
