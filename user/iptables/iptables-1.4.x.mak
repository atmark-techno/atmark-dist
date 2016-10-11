SRC_DIR = iptables-1.4.14
CONF_BUILD = $(shell $(SRC_DIR)/build-aux/config.guess)
CONF_HOST = $(shell $(SRC_DIR)/build-aux/config.sub $(CROSS:-=))

BUILD_DIR = builddir

DEST_DIR = destdir

CONFIGURE_OPT = --build=$(CONF_BUILD)		\
		--host=$(CONF_HOST)		\
		--prefix=/usr			\
		--with-xtlibdir=/lib/xtables	\
		--libdir=/lib			\
		--enable-shared			\
		--disable-libipq
ifeq ($(CONFIG_USER_IPTABLES_IP6TABLES),y)
CONFIGURE_OPT += --enable-ipv6
else
CONFIGURE_OPT += --disable-ipv6
endif

all: build

$(BUILD_DIR)/Makefile: $(CONFIG_CONFIG)
	mkdir -p $(BUILD_DIR)
	(cd $(BUILD_DIR); ../$(SRC_DIR)/configure $(CONFIGURE_OPT))

build: $(BUILD_DIR)/Makefile
	make -C $(BUILD_DIR)

install: build
	[ -d $(DEST_DIR) ] || mkdir -p $(DEST_DIR)
	DESTDIR=$(shell pwd)/$(DEST_DIR) make -C $(BUILD_DIR) install

romfs: install
	$(ROMFSINST) $(DEST_DIR)/usr/sbin/xtables-multi \
		/usr/sbin/xtables-multi
	$(ROMFSINST) -s xtables-multi /usr/sbin/iptables
	$(ROMFSINST) -s xtables-multi /usr/sbin/iptables-restore
	$(ROMFSINST) -s xtables-multi /usr/sbin/iptables-save
ifeq ($(CONFIG_USER_IPTABLES_IP6TABLES),y)
	$(ROMFSINST) -s xtables-multi /usr/sbin/ip6tables
	$(ROMFSINST) -s xtables-multi /usr/sbin/ip6tables-restore
	$(ROMFSINST) -s xtables-multi /usr/sbin/ip6tables-save
endif
	$(ROMFSINST) -s /usr/sbin/xtables-multi /usr/bin/iptables-xml
	$(ROMFSINST) $(DEST_DIR)/lib/libip4tc.so.0.1.0 /lib/libip4tc.so.0.1.0
	$(ROMFSINST) -s libip4tc.so.0.1.0 /lib/libip4tc.so.0
	$(ROMFSINST) -s libip4tc.so.0.1.0 /lib/libip4tc.so
ifeq ($(CONFIG_USER_IPTABLES_IP6TABLES),y)
	$(ROMFSINST) $(DEST_DIR)/lib/libip6tc.so.0.1.0  /lib/libip6tc.so.0.1.0
	$(ROMFSINST) -s libip6tc.so.0.1.0 /lib/libip6tc.so.0
	$(ROMFSINST) -s libip6tc.so.0.1.0 /lib/libip6tc.so
endif
	$(ROMFSINST) $(DEST_DIR)/lib/libiptc.so.0.0.0  /lib/libiptc.so.0.0.0
	$(ROMFSINST) -s libiptc.so.0.0.0 /lib/libiptc.so.0
	$(ROMFSINST) -s libiptc.so.0.0.0 /lib/libiptc.so
	$(ROMFSINST) $(DEST_DIR)/lib/libxtables.so.7.0.0 \
		/lib/libxtables.so.7.0.0
	$(ROMFSINST) -s libxtables.so.7.0.0 /lib/libxtables.so.7
	$(ROMFSINST) -s libxtables.so.7.0.0 /lib/libxtables.so

	[ -d $(ROMFSDIR)/lib/xtables ] || mkdir -p $(ROMFSDIR)/lib/xtables
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_CLUSTERIP \
		$(DEST_DIR)/lib/xtables/libipt_CLUSTERIP.so \
		/lib/xtables/libipt_CLUSTERIP.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_DNAT \
		$(DEST_DIR)/lib/xtables/libipt_DNAT.so \
		/lib/xtables/libipt_DNAT.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_ECN \
		$(DEST_DIR)/lib/xtables/libipt_ECN.so \
		/lib/xtables/libipt_ECN.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_LOG \
		$(DEST_DIR)/lib/xtables/libipt_LOG.so \
		/lib/xtables/libipt_LOG.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_MASQUERADE \
		$(DEST_DIR)/lib/xtables/libipt_MASQUERADE.so \
		/lib/xtables/libipt_MASQUERADE.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_MIRROR \
		$(DEST_DIR)/lib/xtables/libipt_MIRROR.so \
		/lib/xtables/libipt_MIRROR.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_NETMAP \
		$(DEST_DIR)/lib/xtables/libipt_NETMAP.so \
		/lib/xtables/libipt_NETMAP.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_REDIRECT \
		$(DEST_DIR)/lib/xtables/libipt_REDIRECT.so \
		/lib/xtables/libipt_REDIRECT.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_REJECT \
		$(DEST_DIR)/lib/xtables/libipt_REJECT.so \
		/lib/xtables/libipt_REJECT.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_SAME \
		$(DEST_DIR)/lib/xtables/libipt_SAME.so \
		/lib/xtables/libipt_SAME.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_SNAT \
		$(DEST_DIR)/lib/xtables/libipt_SNAT.so \
		/lib/xtables/libipt_SNAT.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_TTL \
		$(DEST_DIR)/lib/xtables/libipt_TTL.so \
		/lib/xtables/libipt_TTL.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_ULOG \
		$(DEST_DIR)/lib/xtables/libipt_ULOG.so \
		/lib/xtables/libipt_ULOG.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_ah \
		$(DEST_DIR)/lib/xtables/libipt_ah.so \
		/lib/xtables/libipt_ah.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_icmp \
		$(DEST_DIR)/lib/xtables/libipt_icmp.so \
		/lib/xtables/libipt_icmp.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_realm \
		$(DEST_DIR)/lib/xtables/libipt_realm.so \
		/lib/xtables/libipt_realm.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_ttl \
		$(DEST_DIR)/lib/xtables/libipt_ttl.so \
		/lib/xtables/libipt_ttl.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_unclean \
		$(DEST_DIR)/lib/xtables/libipt_unclean.so \
		/lib/xtables/libipt_unclean.so
ifeq ($(CONFIG_USER_IPTABLES_IP6TABLES),y)
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_HL \
		$(DEST_DIR)/lib/xtables/libip6t_HL.so \
		/lib/xtables/libip6t_HL.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_LOG \
		$(DEST_DIR)/lib/xtables/libip6t_LOG.so \
		/lib/xtables/libip6t_LOG.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_REJECT \
		$(DEST_DIR)/lib/xtables/libip6t_REJECT.so \
		/lib/xtables/libip6t_REJECT.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_ah \
		$(DEST_DIR)/lib/xtables/libip6t_ah.so \
		/lib/xtables/libip6t_ah.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_dst \
		$(DEST_DIR)/lib/xtables/libip6t_dst.so \
		/lib/xtables/libip6t_dst.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_eui64 \
		$(DEST_DIR)/lib/xtables/libip6t_eui64.so \
		/lib/xtables/libip6t_eui64.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_frag \
		$(DEST_DIR)/lib/xtables/libip6t_frag.so \
		/lib/xtables/libip6t_frag.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_hbh \
		$(DEST_DIR)/lib/xtables/libip6t_hbh.so \
		/lib/xtables/libip6t_hbh.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_hl \
		$(DEST_DIR)/lib/xtables/libip6t_hl.so \
		/lib/xtables/libip6t_hl.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_icmp6 \
		$(DEST_DIR)/lib/xtables/libip6t_icmp6.so \
		/lib/xtables/libip6t_icmp6.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_ipv6header \
		$(DEST_DIR)/lib/xtables/libip6t_ipv6header.so \
		/lib/xtables/libip6t_ipv6header.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_mh \
		$(DEST_DIR)/lib/xtables/libip6t_mh.so \
		/lib/xtables/libip6t_mh.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_rt \
		$(DEST_DIR)/lib/xtables/libip6t_rt.so \
		/lib/xtables/libip6t_rt.so
endif
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_AUDIT \
		$(DEST_DIR)/lib/xtables/libxt_AUDIT.so \
		/lib/xtables/libxt_AUDIT.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_CHECKSUM \
		$(DEST_DIR)/lib/xtables/libxt_CHECKSUM.so \
		/lib/xtables/libxt_CHECKSUM.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_CLASSIFY \
		$(DEST_DIR)/lib/xtables/libxt_CLASSIFY.so \
		/lib/xtables/libxt_CLASSIFY.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_CONNMARK \
		$(DEST_DIR)/lib/xtables/libxt_CONNMARK.so \
		/lib/xtables/libxt_CONNMARK.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_CONNSECMARK \
		$(DEST_DIR)/lib/xtables/libxt_CONNSECMARK.so \
		/lib/xtables/libxt_CONNSECMARK.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_CT \
		$(DEST_DIR)/lib/xtables/libxt_CT.so \
		/lib/xtables/libxt_CT.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_DSCP \
		$(DEST_DIR)/lib/xtables/libxt_DSCP.so \
		/lib/xtables/libxt_DSCP.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_IDLETIMER \
		$(DEST_DIR)/lib/xtables/libxt_IDLETIMER.so \
		/lib/xtables/libxt_IDLETIMER.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_LED \
		$(DEST_DIR)/lib/xtables/libxt_LED.so \
		/lib/xtables/libxt_LED.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_MARK \
		$(DEST_DIR)/lib/xtables/libxt_MARK.so \
		/lib/xtables/libxt_MARK.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_NFLOG \
		$(DEST_DIR)/lib/xtables/libxt_NFLOG.so \
		/lib/xtables/libxt_NFLOG.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_NFQUEUE \
		$(DEST_DIR)/lib/xtables/libxt_NFQUEUE.so \
		/lib/xtables/libxt_NFQUEUE.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_NOTRACK \
		$(DEST_DIR)/lib/xtables/libxt_NOTRACK.so \
		/lib/xtables/libxt_NOTRACK.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_RATEEST \
		$(DEST_DIR)/lib/xtables/libxt_RATEEST.so \
		/lib/xtables/libxt_RATEEST.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_SECMARK \
		$(DEST_DIR)/lib/xtables/libxt_SECMARK.so \
		/lib/xtables/libxt_SECMARK.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_SET \
		$(DEST_DIR)/lib/xtables/libxt_SET.so \
		/lib/xtables/libxt_SET.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_TCPMSS \
		$(DEST_DIR)/lib/xtables/libxt_TCPMSS.so \
		/lib/xtables/libxt_TCPMSS.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_TCPOPTSTRIP \
		$(DEST_DIR)/lib/xtables/libxt_TCPOPTSTRIP.so \
		/lib/xtables/libxt_TCPOPTSTRIP.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_TEE \
		$(DEST_DIR)/lib/xtables/libxt_TEE.so \
		/lib/xtables/libxt_TEE.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_TOS \
		$(DEST_DIR)/lib/xtables/libxt_TOS.so \
		/lib/xtables/libxt_TOS.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_TPROXY \
		$(DEST_DIR)/lib/xtables/libxt_TPROXY.so \
		/lib/xtables/libxt_TPROXY.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_TRACE \
		$(DEST_DIR)/lib/xtables/libxt_TRACE.so \
		/lib/xtables/libxt_TRACE.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_addrtype \
		$(DEST_DIR)/lib/xtables/libxt_addrtype.so \
		/lib/xtables/libxt_addrtype.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_cluster \
		$(DEST_DIR)/lib/xtables/libxt_cluster.so \
		/lib/xtables/libxt_cluster.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_comment \
		$(DEST_DIR)/lib/xtables/libxt_comment.so \
		/lib/xtables/libxt_comment.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_connbytes \
		$(DEST_DIR)/lib/xtables/libxt_connbytes.so \
		/lib/xtables/libxt_connbytes.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_connlimit \
		$(DEST_DIR)/lib/xtables/libxt_connlimit.so \
		/lib/xtables/libxt_connlimit.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_connmark \
		$(DEST_DIR)/lib/xtables/libxt_connmark.so \
		/lib/xtables/libxt_connmark.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_conntrack \
		$(DEST_DIR)/lib/xtables/libxt_conntrack.so \
		/lib/xtables/libxt_conntrack.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_cpu \
		$(DEST_DIR)/lib/xtables/libxt_cpu.so \
		/lib/xtables/libxt_cpu.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_dccp \
		$(DEST_DIR)/lib/xtables/libxt_dccp.so \
		/lib/xtables/libxt_dccp.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_devgroup \
		$(DEST_DIR)/lib/xtables/libxt_devgroup.so \
		/lib/xtables/libxt_devgroup.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_dscp \
		$(DEST_DIR)/lib/xtables/libxt_dscp.so \
		/lib/xtables/libxt_dscp.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_ecn \
		$(DEST_DIR)/lib/xtables/libxt_ecn.so \
		/lib/xtables/libxt_ecn.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_esp \
		$(DEST_DIR)/lib/xtables/libxt_esp.so \
		/lib/xtables/libxt_esp.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_hashlimit \
		$(DEST_DIR)/lib/xtables/libxt_hashlimit.so \
		/lib/xtables/libxt_hashlimit.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_helper \
		$(DEST_DIR)/lib/xtables/libxt_helper.so \
		/lib/xtables/libxt_helper.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_iprange \
		$(DEST_DIR)/lib/xtables/libxt_iprange.so \
		/lib/xtables/libxt_iprange.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_ipvs \
		$(DEST_DIR)/lib/xtables/libxt_ipvs.so \
		/lib/xtables/libxt_ipvs.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_length \
		$(DEST_DIR)/lib/xtables/libxt_length.so \
		/lib/xtables/libxt_length.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_limit \
		$(DEST_DIR)/lib/xtables/libxt_limit.so \
		/lib/xtables/libxt_limit.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_mac \
		$(DEST_DIR)/lib/xtables/libxt_mac.so \
		/lib/xtables/libxt_mac.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_mark \
		$(DEST_DIR)/lib/xtables/libxt_mark.so \
		/lib/xtables/libxt_mark.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_multiport \
		$(DEST_DIR)/lib/xtables/libxt_multiport.so \
		/lib/xtables/libxt_multiport.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_nfacct \
		$(DEST_DIR)/lib/xtables/libxt_nfacct.so \
		/lib/xtables/libxt_nfacct.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_osf \
		$(DEST_DIR)/lib/xtables/libxt_osf.so \
		/lib/xtables/libxt_osf.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_owner \
		$(DEST_DIR)/lib/xtables/libxt_owner.so \
		/lib/xtables/libxt_owner.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_physdev \
		$(DEST_DIR)/lib/xtables/libxt_physdev.so \
		/lib/xtables/libxt_physdev.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_pkttype \
		$(DEST_DIR)/lib/xtables/libxt_pkttype.so \
		/lib/xtables/libxt_pkttype.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_policy \
		$(DEST_DIR)/lib/xtables/libxt_policy.so \
		/lib/xtables/libxt_policy.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_quota \
		$(DEST_DIR)/lib/xtables/libxt_quota.so \
		/lib/xtables/libxt_quota.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_rateest \
		$(DEST_DIR)/lib/xtables/libxt_rateest.so \
		/lib/xtables/libxt_rateest.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_recent \
		$(DEST_DIR)/lib/xtables/libxt_recent.so \
		/lib/xtables/libxt_recent.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_rpfilter \
		$(DEST_DIR)/lib/xtables/libxt_rpfilter.so \
		/lib/xtables/libxt_rpfilter.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_sctp \
		$(DEST_DIR)/lib/xtables/libxt_sctp.so \
		/lib/xtables/libxt_sctp.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_set \
		$(DEST_DIR)/lib/xtables/libxt_set.so \
		/lib/xtables/libxt_set.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_socket \
		$(DEST_DIR)/lib/xtables/libxt_socket.so \
		/lib/xtables/libxt_socket.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_standard \
		$(DEST_DIR)/lib/xtables/libxt_standard.so \
		/lib/xtables/libxt_standard.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_state \
		$(DEST_DIR)/lib/xtables/libxt_state.so \
		/lib/xtables/libxt_state.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_statistic \
		$(DEST_DIR)/lib/xtables/libxt_statistic.so \
		/lib/xtables/libxt_statistic.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_string \
		$(DEST_DIR)/lib/xtables/libxt_string.so \
		/lib/xtables/libxt_string.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_tcp \
		$(DEST_DIR)/lib/xtables/libxt_tcp.so \
		/lib/xtables/libxt_tcp.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_tcpmss \
		$(DEST_DIR)/lib/xtables/libxt_tcpmss.so \
		/lib/xtables/libxt_tcpmss.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_time \
		$(DEST_DIR)/lib/xtables/libxt_time.so \
		/lib/xtables/libxt_time.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_tos \
		$(DEST_DIR)/lib/xtables/libxt_tos.so \
		/lib/xtables/libxt_tos.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_u32 \
		$(DEST_DIR)/lib/xtables/libxt_u32.so \
		/lib/xtables/libxt_u32.so
	$(ROMFSINST) -e CONFIG_USER_IPTABLES_udp \
		$(DEST_DIR)/lib/xtables/libxt_udp.so \
		/lib/xtables/libxt_udp.so

clean:
	[ -d $(BUILD_DIR) ] && make -C $(BUILD_DIR) clean
	rm -rf $(DEST_DIR)

distclean:
	rm -rf $(BUILD_DIR)
	rm -rf $(DEST_DIR)
