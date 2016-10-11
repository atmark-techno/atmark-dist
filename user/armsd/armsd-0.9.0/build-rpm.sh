#!/bin/sh

DEBUG=0
if [ x"$1" = x"debug" ]; then
	DEBUG=1
fi

pkg=armsd
repo=git://giti.tokyo.iiji.jp/arms/armsd2.git

ORIGPWD=`pwd`
TMPDIR=`mktemp -d`
SPECFILE=$TMPDIR/$pkg.spec

rm -rf $TMPDIR
mkdir -p $TMPDIR
cp $pkg.spec $SPECFILE
cd $TMPDIR

RPMROOT=$TMPDIR/rpmroot

mkdir -p $RPMROOT/BUILD
mkdir -p $RPMROOT/RPMS
mkdir -p $RPMROOT/SOURCES
mkdir -p $RPMROOT/SPECS

cp $SPECFILE $RPMROOT/SPECS
git clone --depth=1 $repo $pkg

revision=`git --git-dir=$pkg/.git log -n1 --pretty=format:%h`
version=`awk '/^%define version/ { print $3; exit }' $SPECFILE`
if [ $DEBUG -ne 0 ]; then
	sed -i "s/^Release:.*/Release: $revision/" $RPMROOT/SPECS/$pkg.spec
fi

mv -i $pkg $pkg-$version
tar czf $pkg-$version.tar.gz --exclude=.git $pkg-$version
mv $pkg-$version.tar.gz $RPMROOT/SOURCES/

cd $RPMROOT/SPECS/
rpmbuild -bb --define "_topdir $RPMROOT" $RPMROOT/SPECS/$pkg.spec

echo
echo ===========================
echo \* RPM packages built:
ls -1 $RPMROOT/RPMS/*
cp $RPMROOT/RPMS/*/* $ORIGPWD
rm -rf $TMPDIR
