# Makefile for the ELKS images

include Make.defs
include Make.rules

# Template directory for installing applications and linux before genfs
TEMPLATE_DIR = $(ELKSCMD_DIR)/rootfs_template

# TARGET_MNT is used in elkscmd/* Makefiles for install prefix
TARGET_MNT = $(TEMPLATE_DIR)
export TARGET_MNT

# add image block size to mkfs options for mfs
MFS_OPTS=$(MKFS_OPTS) -s$(TARGET_BLKS) -k
#VERBOSE=-v

# command to make char/block devices in image (no sudo required)
MKDEV=mfs $(TARGET_FILE) mknod

#
# Determine what to build for All target
TARGETS=
ifdef CONFIG_IMG_BOOT
TARGETS += $(FD_BSECT) $(ELKS_DIR)/arch/i86/boot/Image
endif
TARGETS += template image
ifdef CONFIG_IMG_LINK
TARGETS += devices
endif
ifdef CONFIG_IMG_ROM
TARGETS += romfs
endif

all: $(TARGETS)
	mfsck -fv $(TARGET_FILE)
	mfs $(TARGET_FILE) stat
#	mfs $(TARGET_FILE) ls -l / /bin /usr/bin /dev /etc
#	cp $(TARGET_FILE) ~greg/net/bootimages/elks/test2.img

#
# Create character and block devices in image (make target devices:)
include Make.devices

#
# Populate template for filesystem genfs
template:
	$(MAKE) -C $(ELKSCMD_DIR) install
	bash -c "$(ELKSCMD_DIR)/tools/ver.pl $(ELKS_DIR)/Makefile-rules > $(TEMPLATE_DIR)/etc/issue"
ifdef CONFIG_IMG_BOOT
	install $(ELKS_DIR)/arch/i86/boot/Image $(TEMPLATE_DIR)/linux
endif

#
# Create MINIX filesystem from template
image:
	mfs $(VERBOSE) $(TARGET_FILE) genfs $(MFS_OPTS) $(TEMPLATE_DIR)
ifdef CONFIG_IMG_BOOT
	mfs $(TARGET_FILE) boot $(FD_BSECT)
endif

romfs:
	mkromfs $(TEMPLATE_DIR)

# Clean target
clean:
	-rm -f $(TARGET_FILE)
	-rm -f $(TEMPLATE_DIR)/linux
	-rm -f $(TEMPLATE_DIR)/bin/*
	-rm -f $(TEMPLATE_DIR)/sbin/*
	-rm -f $(TEMPLATE_DIR)/usr/bin/*
	-rm -f $(TEMPLATE_DIR)/etc/issue
	-rm -f $(TEMPLATE_DIR)/etc/resolv.conf
	-rm -f $(TEMPLATE_DIR)/dev/*
	-rm -rf $(TEMPLATE_DIR)/var/www
	-rm -f *.bin

#
# The following is the original installation mechanism,
# could be used for systems requiring mount device
orgall:
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

orgclean:
	-sudo umount -d $(TARGET_MNT) || true
	sudo rm -rf $(TARGET_MNT)
	sudo rm -rf *.bin
