
PROCPS_VER = 3.2.7
SRC_DIR = procps-$(PROCPS_VER)

TARGET_y = 
TARGET_$(CONFIG_USER_PROCPS_FREE)	+= /usr/bin/free
TARGET_$(CONFIG_USER_PROCPS_KILL)	+= /bin/kill
TARGET_$(CONFIG_USER_PROCPS_PGREP)	+= /usr/bin/pgrep
TARGET_$(CONFIG_USER_PROCPS_PKILL)	+= /usr/bin/pkill
TARGET_$(CONFIG_USER_PROCPS_PMAP)	+= /usr/bin/pmap
TARGET_$(CONFIG_USER_PROCPS_PS)		+= /bin/ps
TARGET_$(CONFIG_USER_PROCPS_PWDX)	+= /usr/bin/pwdx
TARGET_$(CONFIG_USER_PROCPS_SKILL)	+= /usr/bin/skill
TARGET_$(CONFIG_USER_PROCPS_SLABTOP)	+= /usr/bin/slabtop
TARGET_$(CONFIG_USER_PROCPS_SNICE)	+= /usr/bin/snice
TARGET_$(CONFIG_USER_PROCPS_SYSCTL)	+= /sbin/sysctl
TARGET_$(CONFIG_USER_PROCPS_TLOAD)	+= /usr/bin/tload
TARGET_$(CONFIG_USER_PROCPS_TOP)	+= /usr/bin/top
TARGET_$(CONFIG_USER_PROCPS_UPTIME)	+= /usr/bin/uptime
TARGET_$(CONFIG_USER_PROCPS_VMSTAT)	+= /usr/bin/vmstat
TARGET_$(CONFIG_USER_PROCPS_W)		+= /usr/bin/w
TARGET_$(CONFIG_USER_PROCPS_WATCH)	+= /usr/bin/watch

build:
	make -C $(SRC_DIR) BINFILES="$(TARGET_y)"

romfs: build
	for target in $(TARGET_y); do \
		if [ "`basename $$target`" = "ps" ]; then \
			$(ROMFSINST) $(SRC_DIR)/ps/ps $$target; \
		elif [ "`basename $$target`" = "w" ]; then \
			$(ROMFSINST) $(SRC_DIR)/w $$target.procps; \
		else \
			$(ROMFSINST) $(SRC_DIR)/`basename $$target` $$target; \
		fi;\
	done
	$(ROMFSINST) $(SRC_DIR)/proc/libproc-$(PROCPS_VER).so /lib/libproc-$(PROCPS_VER).so

clean:
	make -C $(SRC_DIR) clean

