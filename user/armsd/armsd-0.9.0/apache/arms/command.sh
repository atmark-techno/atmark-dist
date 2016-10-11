#!/bin/sh

id=$1
in=$2
out=$3

echo invoked with: $0 $*
cd `dirname $0`
. ./params.sh

echo input parameter:
cat -n $in

echo 'jikkou sita tsumori :)' >$out
