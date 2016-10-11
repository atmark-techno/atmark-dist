#!/bin/sh
#VERSION="1.1.0"

TARGET_LIBS=
TARGET_FILES=${TARGET_FILES:="group passwd"}
TARGET_DIRS=${TARGET_DIRS:="bin@511 lib@511 etc@511 pub@777"}
LS_REAL=${LS_REAL:=/bin/ls}

FTP_DIR="/home/ftp"
OUTPUT_FILE="${ROMFSDIR}/etc/init.d/checkftp"
CROSS_OBJDUMP="$CROSS_COMPILE"objdump

TARGET_LIB_DIR="/usr/${CROSS_COMPILE%%-}/lib"

check_library(){
	for i in `$CROSS_OBJDUMP -p $1 | grep NEEDED | sed -e "s/NEEDED//"`
	do
		UNIQUE=true;
 		for j in ${TARGET_LIBS}
 		do
 			if [ ${i} = ${j} ]; then
				UNIQUE=false;
				break
			fi
		done

		if [ ${UNIQUE} = true ]; then
			TARGET_LIBS="${TARGET_LIBS} ${i}"
		fi
	done
}

check_library ${ROOTDIR}/user/busybox/busybox
for i in ${TARGET_LIBS}
do
  check_library ${TARGET_LIB_DIR}/${i}
done

if [ "${ROMFSDIR}" = "" ]; then
	echo "ROMFSDIR is not set!" >&2
	exit 1
fi

exec >${OUTPUT_FILE}

cat <<EOF
#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin

EOF

if [ ${DO_CHECK_STATUS} = 1 ] ;then
cat <<EOF
. /etc/init.d/functions

echo -n "Configure /home/ftp: "
EOF
else
cat <<EOF
echo "Configure /home/ftp: "
EOF
fi

cat <<EOF
[ -f /home/ftp/bin/ls ] || (
	ln ${LS_REAL} ${FTP_DIR}/bin/ls &&
EOF

for lib in ${TARGET_LIBS}
do
SRC=`readlink ${TARGET_LIB_DIR}/${lib}`
if [ -z $SRC ]; then
cat <<EOF
	ln /lib/${lib} ${FTP_DIR}/lib/${lib} &&
EOF
else
cat <<EOF
	ln /lib/$SRC ${FTP_DIR}/lib/${lib} &&
EOF
fi
done

for file in ${TARGET_FILES}
do
cat <<EOF
	ln /etc/${file} ${FTP_DIR}/etc/${file} &&
EOF
done

for dir in ${TARGET_DIRS}
do
cat <<EOF
	chmod ${dir#*@} ${FTP_DIR}/${dir%@*} &&
EOF
done

cat <<EOF
	true)
EOF

if [ ${DO_CHECK_STATUS} = 1 ] ;then
cat <<EOF

check_status
EOF
fi

chmod a+x ${OUTPUT_FILE} >&2

exit 0
