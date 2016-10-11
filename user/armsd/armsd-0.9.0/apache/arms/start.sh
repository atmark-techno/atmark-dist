#!/bin/sh

id=$1
version=$2
info=$3
config=$4

echo invoked with: $0 $*
cd `dirname $0`
. ./params.sh 

cp $config $httpdconf
$apctl -f $httpdconf -k stop	# to be sure
sleep 1
$apctl -f $httpdconf -k start 
