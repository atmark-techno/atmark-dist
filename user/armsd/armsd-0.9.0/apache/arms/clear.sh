#!/bin/sh

id=$1
in=$2
out=$3

echo invoked with: $0 $*
cd `dirname $0`
. ./params.sh 

echo 'status cleared...' >$out
