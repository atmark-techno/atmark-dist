SRC_DIR = ppp-2.4.4

PPP_DIR = $(shell pwd)
INSTALL_DIR = preinstall

SBIN_DIR = sbin
LIB_DIR = lib/pppd/2.4.4
ETC_DIR = etc/ppp

TARGET = \
	chat \
	pppd \
	pppdump \
	pppoe-discovery \
	pppstats

TARGET_LIB = \
	minconn.so \
	passwordfd.so \
	radattr.so \
	radrealms.so \
	winbind.so \
	passprompt.so \
	pppoatm.so \
	radius.so \
	rp-pppoe.so

TARGET_ETC = \
	chap-secrets \
	options \
	pap-secrets \
	ppp-on-dialer

all: build

build:
	make -C $(SRC_DIR) all
	mkdir -p $(INSTALL_DIR)
	make -C $(SRC_DIR) \
		DESTDIR=$(PPP_DIR)/$(INSTALL_DIR) \
		ETCDIR=$(PPP_DIR)/$(INSTALL_DIR)/etc/ppp \
		install install-etcppp

romfs: build
	for target in $(TARGET); do \
		$(ROMFSINST) "$(INSTALL_DIR)/$(SBIN_DIR)/$$target" "/$(SBIN_DIR)/$$target"; \
	done

	mkdir -p $(ROMFSDIR)/$(LIB_DIR)
	for target in $(TARGET_LIB); do \
	    $(ROMFSINST) "$(INSTALL_DIR)/$(LIB_DIR)/$$target" "/$(LIB_DIR)/$$target"; \
	done

	mkdir -p $(ROMFSDIR)/$(ETC_DIR)
	for target in $(TARGET_ETC); do \
	    $(ROMFSINST) "$(INSTALL_DIR)/$(ETC_DIR)/$$target" "/$(ETC_DIR)/$$target"; \
	done

clean:
	make -C $(SRC_DIR) clean

distclean: clean
	rm -rf $(INSTALL_DIR)
