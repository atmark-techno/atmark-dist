SRC_DIR = iproute2-3.16.0

BUILD_DIR = builddir

all: build

$(BUILD_DIR)/Config: $(CONFIG_CONFIG)
	rm -rf $(BUILD_DIR)
	(cd $(SRC_DIR); \
	 find . -type d > ../.dirs; \
	 find . ! -type d | grep -v ./makefile > ../.files)
	while read t; do mkdir -p $(BUILD_DIR)/$$t; done < .dirs
	while read t; do ln -s `pwd`/$(SRC_DIR)/$$t $(BUILD_DIR)/$$t; \
		done < .files
	rm -f .dirs .files
	echo "PKG_CONFIG:=pkg-config" > $(BUILD_DIR)/Config; \
	echo "AR:=$(AR)" >> $(BUILD_DIR)/Config
	echo "CC:=$(CC)" >> $(BUILD_DIR)/Config
ifeq ($(findstring y, $(CONFIG_USER_IPTABLES_IPTABLES_1_4_8) \
		      $(CONFIG_USER_IPTABLES_IPTABLES_1_4_14)), y)
	echo "TC_CONFIG_XT:=y" >> $(BUILD_DIR)/Config
	echo "IPT_LIB_DIR:=/lib/xtables" >> $(BUILD_DIR)/Config
endif

build: $(BUILD_DIR)/Config
ifeq ($(findstring y, $(CONFIG_USER_IPTABLES_IPTABLES_1_4_8) \
		      $(CONFIG_USER_IPTABLES_IPTABLES_1_4_14)), y)
	make -C ../iptables
endif
	make -C $(BUILD_DIR)

romfs: build
	$(ROMFSINST) -e CONFIG_USER_IPROUTE2_IP_IP $(BUILD_DIR)/ip/ip /bin/ip
	$(ROMFSINST) -e CONFIG_USER_IPROUTE2_IP_IP -s /bin/ip /sbin/ip
	$(ROMFSINST) -e CONFIG_USER_IPROUTE2_IP_RTMON \
		$(BUILD_DIR)/ip/rtmon /sbin/rtmon
ifeq ($(CONFIG_USER_IPROUTE2_IP_ROUTEF), y)
	$(ROMFSINST) $(BUILD_DIR)/ip/routef /usr/bin/routef
	chmod +x $(ROMFSDIR)/usr/bin/routef
endif
ifeq ($(CONFIG_USER_IPROUTE2_IP_ROUTEL), y)
	$(ROMFSINST) $(BUILD_DIR)/ip/routel /usr/bin/routel
	chmod +x $(ROMFSDIR)/usr/bin/routel
endif

ifeq ($(CONFIG_USER_IPROUTE2_TC_TC), y)
	$(ROMFSINST) $(BUILD_DIR)/tc/tc /sbin/tc

	mkdir -p $(ROMFSDIR)/etc/iproute2
	$(ROMFSINST) $(SRC_DIR)/etc/iproute2/ematch_map \
		/etc/iproute2/ematch_map

	mkdir -p $(ROMFSDIR)/usr/lib/tc
	$(ROMFSINST) $(BUILD_DIR)/tc/m_xt.so /usr/lib/tc/m_xt.so
	$(ROMFSINST) -s m_xt.so /usr/lib/tc/m_ipt.so
	$(ROMFSINST) $(BUILD_DIR)/netem/experimental.dist \
		/usr/lib/tc/experimental.dist
	$(ROMFSINST) $(BUILD_DIR)/netem/normal.dist \
		/usr/lib/tc/normal.dist
	$(ROMFSINST) $(BUILD_DIR)/netem/pareto.dist \
		/usr/lib/tc/pareto.dist
	$(ROMFSINST) $(BUILD_DIR)/netem/paretonormal.dist \
		/usr/lib/tc/paretonormal.dist
endif

	$(ROMFSINST) -e CONFIG_USER_IPROUTE2_BRIDGE_BRIDGE \
		$(BUILD_DIR)/bridge/bridge /sbin/bridge

	$(ROMFSINST) -e CONFIG_USER_IPROUTE2_MISC_SS \
		$(BUILD_DIR)/misc/ss /bin/ss
	$(ROMFSINST) -e CONFIG_USER_IPROUTE2_MISC_RTACCT \
		$(BUILD_DIR)/misc/rtacct /sbin/rtacct
	$(ROMFSINST) -e CONFIG_USER_IPROUTE2_MISC_LNSTAT \
		$(BUILD_DIR)/misc/lnstat /usr/bin/lnstat
	$(ROMFSINST) -e CONFIG_USER_IPROUTE2_MISC_LNSTAT \
		-s lnstat /usr/bin/ctstat
	$(ROMFSINST) -e CONFIG_USER_IPROUTE2_MISC_LNSTAT \
		-s lnstat /usr/bin/rtstat
	$(ROMFSINST) -e CONFIG_USER_IPROUTE2_MISC_NSTAT \
		$(BUILD_DIR)/misc/nstat /usr/bin/nstat
ifeq ($(CONFIG_USER_IPROUTE2_MISC_ARPD), y)
	$(ROMFSINST) $(BUILD_DIR)/misc/arpd /usr/sbin/arpd
	mkdir -p $(ROMFSDIR)/var/lib/arpd
endif

ifeq ($(findstring y, $(CONFIG_USER_IPROUTE2_IP_IP) \
		      $(CONFIG_USER_IPROUTE2_TC_TC) \
		      $(CONFIG_USER_IPROUTE2_MISC_RTACCT)), y)
	mkdir -p $(ROMFSDIR)/etc/iproute2
	$(ROMFSINST) $(SRC_DIR)/etc/iproute2/group \
		/etc/iproute2/group
	$(ROMFSINST) $(SRC_DIR)/etc/iproute2/rt_dsfield \
		/etc/iproute2/rt_dsfield
	$(ROMFSINST) $(SRC_DIR)/etc/iproute2/rt_protos \
		/etc/iproute2/rt_protos
	$(ROMFSINST) $(SRC_DIR)/etc/iproute2/rt_realms \
		/etc/iproute2/rt_realms
	$(ROMFSINST) $(SRC_DIR)/etc/iproute2/rt_scopes \
		/etc/iproute2/rt_scopes
	$(ROMFSINST) $(SRC_DIR)/etc/iproute2/rt_tables \
		/etc/iproute2/rt_tables
endif

clean:
	rm -rf $(BUILD_DIR)

distclean: clean
