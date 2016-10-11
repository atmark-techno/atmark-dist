#!/bin/sh

. /etc/config/armsd.conf.secrets

CONF=/etc/armsd/armsd.conf

cat > $CONF <<- EOF
distribution-id:	$DIST_ID
ls-sa-key:		$LS_SA_KEY

#path-iconfig:		/etc/armsd/initial-config
path-state-cache:	/etc/armsd/armsd.cache
#https-proxy-url:	http://192.168.0.1:8080/

hb-disk-usage0:		/
hb-traffic-if0:		eth0

### see sample scripts in /usr/share/armsd-X.X/examples
script-app-event:	/etc/armsd/scripts/app-event
script-clear: 		/etc/armsd/scripts/clear
script-command: 	/etc/armsd/scripts/command
script-post-pull:	/etc/armsd/scripts/post-pull
script-reconfig:	/etc/armsd/scripts/reconfig
script-start:		/etc/armsd/scripts/start
#script-status:		/etc/armsd/scripts/status
#script-stop:		/etc/armsd/scripts/stop
script-reboot:		/etc/armsd/scripts/reboot
script-line-ctrl:	/etc/armsd/scripts/line
script-state-changed:	/etc/armsd/scripts/state-changed
EOF
