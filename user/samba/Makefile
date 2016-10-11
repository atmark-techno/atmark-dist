
.EXPORT_ALL_VARIABLES:

DIRS = source

all:
	for i in $(DIRS) ; do make -C $$i || exit $? ; done

romfs:
	for i in $(DIRS) ; do make -C $$i romfs || exit $? ; done
	$(ROMFSINST) -e CONFIG_USER_SAMBA \
		-a "netbios-ssn stream tcp nowait root smbd -d1" /etc/inetd.conf

clean:
	-for i in $(DIRS) ; do make -C $$i clean ; done


