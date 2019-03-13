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

