#!/bin/sh

id=$1

echo invoked with: $0 $*
cd `dirname $0`
. ./params.sh 

$apctl -d $apdir -k stop
