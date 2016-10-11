#! /bin/sh

N=/etc/init.d/sudo

set -e

case "$1" in
  start)
	# make sure privileges don't persist across reboots
	if [ -d /var/run/sudo ] && [ "x`ls /var/run/sudo`" != x ]
	then
		touch -t 198501010000 /var/run/sudo/*
	fi
	;;
  stop|reload|restart|force-reload)
	;;
  *)
	echo "Usage: $N {start|stop|restart|force-reload}" >&2
	exit 1
	;;
esac

exit 0
