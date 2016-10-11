
SRC_DIR_$(CONFIG_USER_OPENSSH_VER_6_0P1) = openssh-6.0p1
SRC_DIR_$(CONFIG_USER_OPENSSH_VER_4_3P2) = openssh-4.3p2
SRC_DIR = $(SRC_DIR_y)

CONF_BUILD = $(shell $(SRC_DIR)/config.guess)
CONF_HOST = $(shell $(SRC_DIR)/config.sub $(CROSS:-=))
CONF_OPT = 
CONF_OPT_MISC = 

BIN_y =
BIN_$(CONFIG_USER_OPENSSH_SSH)		+= /bin/ssh
BIN_$(CONFIG_USER_OPENSSH_SSHD)		+= /sbin/sshd
BIN_$(CONFIG_USER_OPENSSH_SSHKEYGEN)	+= /bin/ssh-keygen
BIN_$(CONFIG_USER_OPENSSH_SSHKEYSCAN)	+= /bin/ssh-keyscan
BIN_$(CONFIG_USER_OPENSSH_SSHKEYSIGN)	+= /lib/openssh/ssh-keysign
BIN_$(CONFIG_USER_OPENSSH_SSHRAND)	+= /lib/openssh/ssh-rand-helper
BIN_$(CONFIG_USER_OPENSSH_SSHAGENT)	+= /bin/ssh-agent
BIN_$(CONFIG_USER_OPENSSH_SSHADD)	+= /bin/ssh-add
BIN_$(CONFIG_USER_OPENSSH_SFTPSERVER)	+= /usr/lib/openssh/sftp-server
BIN_$(CONFIG_USER_OPENSSH_SFTP)		+= /bin/sftp
BIN_$(CONFIG_USER_OPENSSH_SCP)		+= /bin/scp
BIN_$(CONFIG_USER_OPENSSH_SSHPKCS11)	+= /bin/ssh-pkcs11-helper
BIN_$(CONFIG_USER_OPENSSH_SSHVULNKEY)	+= /bin/ssh-vulnkey

all: build

$(SRC_DIR)/Makefile:
	(cd $(SRC_DIR); \
	 ./configure --build=$(CONF_BUILD) --host=$(CONF_HOST) \
	             $(CONF_OPT) $(CONF_OPT_MISC));\

build: $(SRC_DIR)/Makefile
	make -C $(SRC_DIR) LD=$(CC)

romfs: build
	[ "$(CONFIG_USER_OPENSSH_SSHD)" != "y" ] || \
		mkdir -p $(ROMFSDIR)/usr/; \
		mkdir -p $(ROMFSDIR)/usr/lib/; \
		mkdir -p $(ROMFSDIR)/usr/lib/openssh

	for target in $(BIN_y); do \
		$(ROMFSINST) $(SRC_DIR)/`basename $$target` $$target; \
	done

clean:
	if [ -f $(SRC_DIR)/Makefile ]; then \
		make -C $(SRC_DIR) clean ;\
	fi

distclean: clean
	if [ -f $(SRC_DIR)/Makefile ]; then \
		make -C $(SRC_DIR) distclean ;\
	fi
