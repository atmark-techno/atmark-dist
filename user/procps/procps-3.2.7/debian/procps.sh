#! /bin/sh
# /etc/init.d/procps: Set kernel variables from /etc/sysctl.conf
#
# written by Elrond <Elrond@Wunder-Nett.org>

### BEGIN INIT INFO
# Provides:          procps
# Required-Start:    mountkernfs
# Required-Stop:
# Default-Start:     S
# Default-Stop:
### END INIT INFO


# Check for existance of the default file and exit if not there,
# Closes #52839 for the boot-floppy people
[ -r /etc/default/rcS ] || exit 0
. /etc/default/rcS
. /lib/lsb/init-functions

PATH=/sbin:$PATH
which sysctl > /dev/null || exit 0


case "$1" in
       start|restart|force-reload)
               if [ ! -r /etc/sysctl.conf ]
               then
                       exit 0
               fi
	       log_action_begin_msg "Setting kernel variables"
               sysctl -q -p
	       log_action_end_msg $?
               ;;
       stop)
               ;;
       *)
               echo "Usage: /etc/init.d/procps.sh {start|stop|restart|force-reload}" >&2
               exit 3
               ;;
esac
exit 0
