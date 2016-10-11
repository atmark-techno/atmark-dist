#!/bin/sh

id=$1
out=$2

echo invoked with: $0 $*
cd `dirname $0`
. ./params.sh 

echo 'debug information...' >$out
