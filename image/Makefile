# Makefile for the ELKS images

include Make.defs
include Make.rules

# All target

ifdef CONFIG_IMG_BOOT
all: $(FD_BSECT) $(ELKS_DIR)/arch/i86/boot/Image
else
all:
endif
ifdef CONFIG_IMG_DEV
	-sudo umount -d $(TARGET_FILE)
endif
	-sudo rm -rf $(TARGET_FILE)
ifdef CONFIG_IMG_DEV
	dd if=/dev/zero of=$(TARGET_FILE) bs=1024 count=$(TARGET_BLKS)
	sudo $(MKFS) $(MKFS_OPTS) $(TARGET_FILE)
endif
	-sudo rm -rf $(TARGET_MNT)
	mkdir -p $(TARGET_MNT)
ifdef CONFIG_IMG_DEV
	sudo mount $(LOOP) $(TARGET_FILE) $(TARGET_MNT)
endif
	(cd $(ELKSCMD_DIR)/rootfs_template; \
		tar cf - --exclude .keep *) | \
		(cd $(TARGET_MNT); sudo tar xpf - --no-same-owner)
	sudo find $(TARGET_MNT) \
		-type d ! -name root -execdir chmod go+rx '{}' \; -o \
		! -type d ! -name .profile -execdir chmod go+r '{}' \;
ifdef CONFIG_IMG_LINK
	(cd $(TARGET_MNT)/dev; sudo ./MAKEDEV; sudo rm MAKEDEV MAKESET)
endif
	$(MAKE) -C $(ELKSCMD_DIR) install
	sudo bash -c "$(ELKSCMD_DIR)/tools/ver.pl $(ELKS_DIR)/Makefile-rules > $(TARGET_MNT)/etc/issue"
ifdef CONFIG_IMG_BOOT
	sudo install $(ELKS_DIR)/arch/i86/boot/Image $(TARGET_MNT)/linux
endif
ifdef CONFIG_IMG_DEV
	sudo umount -d $(TARGET_FILE)
ifdef CONFIG_IMG_BOOT
	dd if=$(FD_BSECT) of=$(TARGET_FILE) bs=512 count=2 conv=notrunc
endif
endif
ifdef CONFIG_IMG_ROM
	cd $(IMGDIR) && sudo $(ELKS_DIR)/tools/mkromfs $(TARGET_MNT)
	sudo chown "`id -u`":"`id -g`" romfs.bin
endif

# Clean target

clean:
	-sudo umount -d $(TARGET_MNT) || true
	sudo rm -rf $(TARGET_MNT)
	sudo rm -rf *.bin

# Proof of concept using mfs and mfsck
MFS_OPTS=$(MKFS_OPTS) -s$(TARGET_BLKS)
MIN_TARGET_FILE=fd1440min.bin

min:
	-rm -rf $(MIN_TARGET_FILE)
ifdef CONFIG_IMG_DEV
	mfs $(MIN_TARGET_FILE) mkfs $(MFS_OPTS)
endif
	mfs $(MIN_TARGET_FILE) mkdir /bin
	mfs $(MIN_TARGET_FILE) mkdir /dev
	mfs $(MIN_TARGET_FILE) mkdir /etc /etc/rc.d
	mfs $(MIN_TARGET_FILE) mkdir /home
	mfs $(MIN_TARGET_FILE) mkdir /lib
	mfs $(MIN_TARGET_FILE) mkdir /mnt
	mfs $(MIN_TARGET_FILE) mkdir /root
	mfs $(MIN_TARGET_FILE) mkdir /sbin
	mfs $(MIN_TARGET_FILE) mkdir /tmp
	mfs $(MIN_TARGET_FILE) mkdir /usr /usr/bin
	mfs $(MIN_TARGET_FILE) mkdir /var /var/run

	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/rootfs_template/etc/group /etc
	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/rootfs_template/etc/inittab /etc
	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/rootfs_template/etc/issue /etc
	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/rootfs_template/etc/passwd /etc
	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/rootfs_template/etc/termcap /etc
	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/rootfs_template/etc/rc.d/rc.sysinit /etc/rc.d
	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/rootfs_template/lib/liberror.txt /lib
	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/rootfs_template/var/run/utmp /var/run
	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/rootfs_template/root/.profile /root

	mfs $(MIN_TARGET_FILE) mknod /dev/fd0 b 3 128
	mfs $(MIN_TARGET_FILE) mknod /dev/tty1 c 4 0
	mfs $(MIN_TARGET_FILE) mknod /dev/ptyp0 c 2 8
	mfs $(MIN_TARGET_FILE) mknod /dev/ttyp0 c 4 8

	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/sys_utils/init /bin
	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/sys_utils/getty /bin
	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/sys_utils/login /bin
	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/file_utils/ls /bin
	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/ash/ash /bin/sh

	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/sh_utils/test /bin
	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/sh_utils/pwd /bin
	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/file_utils/cat /bin
	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/sys_utils/mount /bin
	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/sys_utils/umount /bin

	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/nano-X/bin/demo /usr/bin
	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/nano-X/bin/nterm /usr/bin
	mfs $(MIN_TARGET_FILE) cp $(ELKSCMD_DIR)/nano-X/bin/nclock /usr/bin

#	(cd $(ELKSCMD_DIR)/rootfs_template; \
#		tar cf - --exclude .keep *) | \
#		(cd $(TARGET_MNT); sudo tar xpf - --no-same-owner)
#	sudo find $(TARGET_MNT) \
#		-type d ! -name root -execdir chmod go+rx '{}' \; -o \
#		! -type d ! -name .profile -execdir chmod go+r '{}' \;
ifdef CONFIG_IMG_LINK
#	(cd $(TARGET_MNT)/dev; sudo ./MAKEDEV; sudo rm MAKEDEV MAKESET)
endif
#	$(MAKE) -C $(ELKSCMD_DIR) install

# Create and copy filesystem in a single command, will replace all above
#	mfs -v $(MIN_TARGET_FILE) genfs $(MFS_OPTS) $(ELKSCMD_DIR)/rootfs_template
#

	# note ver.pl broken on OSX
	bash -c "$(ELKSCMD_DIR)/tools/ver.pl $(ELKS_DIR)/Makefile-rules > issue.tmp"
	mfs $(MIN_TARGET_FILE) cp issue.tmp /etc/issue
	rm issue.tmp

ifdef CONFIG_IMG_BOOT
	mfs $(MIN_TARGET_FILE) cp $(ELKS_DIR)/arch/i86/boot/Image /linux
endif
ifdef CONFIG_IMG_DEV
ifdef CONFIG_IMG_BOOT
	mfs $(MIN_TARGET_FILE) boot $(FD_BSECT)
endif
endif
#	mfs $(MIN_TARGET_FILE) ls -l / /etc /etc/rc.d /usr /var /root /dev /bin
	mfs $(MIN_TARGET_FILE) stat
	mfsck -fv $(MIN_TARGET_FILE)
#	cp $(MIN_TARGET_FILE) ~greg/net/bootimages/elks/test2.img
ifdef CONFIG_IMG_ROM
#	cd $(IMGDIR) && sudo $(ELKS_DIR)/tools/mkromfs $(TARGET_MNT)
#	sudo chown "`id -u`":"`id -g`" romfs.bin
endif

minclean:
	rm -rf $(MIN_TARGET_FILE)
	rm -rf *.bin
