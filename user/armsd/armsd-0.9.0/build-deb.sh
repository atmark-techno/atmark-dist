#!/bin/sh

REPO=git://giti.tokyo.iiji.jp/arms/armsd2.git

set -e
set -x

VERS=`head -n 1 debian/changelog | cut -d'(' -f2 | cut -d')' -f1 | cut -d'-' -f1`
REL=`head -n 1 debian/changelog | cut -d'(' -f2 | cut -d')' -f1 | cut -d'-' -f2`
PKGNAME=`head -n 1 debian/changelog | cut -d' ' -f1`
CURDIR=`pwd`

if [ -e /etc/redhat-release ] ; then
	MKTEMP="mktemp -d -p /tmp"
else
	MKTEMP="mktemp -d -t"
fi
TMPDIR=`${MKTEMP} ${PKGNAME}.XXXXXX`

DIRNAME=${PKGNAME}-${VERS}

cd ${TMPDIR}
git clone ${REPO} ${PKGNAME}-${VERS}

mv ${TMPDIR}/${DIRNAME}/debian ${TMPDIR}
rm -rf ${TMPDIR}/${DIRNAME}/.git


tar -czf ${PKGNAME}_${VERS}.orig.tar.gz ${DIRNAME}
mv debian ${TMPDIR}/${DIRNAME}
cd ${DIRNAME}
if [ "${1}" = "--sign" ] ; then
	dpkg-buildpackage
else
	dpkg-buildpackage -uc -us
fi

cd ${CURIDR}
mv ${TMPDIR}/${PKGNAME}*_${VERS}*.deb ${CURDIR}
rm -rf ${TMPDIR}
