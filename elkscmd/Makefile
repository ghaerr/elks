include Make.defs

DIRS=disk_utils file_utils sh_utils sys_utils misc_utils sash levee ash 
KHELPER=$(MINIX_BOOT)/minix_elks.bin
FD_BSECT=$(MINIX_BOOT)/minix.bin

all:
	@for i in $(DIRS); do \
	(cd $$i; make) ; \
	done

$(TARGET_FS):
	dd if=/dev/zero of=$(TARGET_FS) bs=1024 count=$(TARGET_BLKS)

mount_rfs: $(TARGET_FS)
	$(MKFS) $(MKFS_OPTS) $(TARGET_FS) $(TARGET_BLKS)
	mount $(TARGET_FS) $(TARGET_MNT) $(LOOP)
	(cd $(ELKSCMD_DIR)/rootfs_template ; tar cf - *) | \
		(cd $(TARGET_MNT) ; tar xpvf -)
	(cd $(TARGET_MNT)/dev ; ./MAKEDEV)
	

rfs: mount_rfs
	@for i in $(DIRS); do \
	(cd $$i; make rfs) ; \
	done
	umount $(TARGET_FS)
	
min_rfs: mount_rfs
	@for i in $(DIRS); do \
	(cd $$i; make min_rfs) ; \
	done
	umount $(TARGET_FS)

$(ELKS_DIR)/.config:
	(cd $(ELKS_DIR) ; make config)

$(ELKS_DIR)/Image: $(ELKS_DIR)/.config
	(cd $(ELKS_DIR) ; make)

$(FD_BSECT):
	(cd $(MINIX_BOOT) ; make bin)

$(KHELPER):
	(cd $(MINIX_BOOT) ; make)

floppy: mount_rfs $(ELKS_DIR)/Image $(FD_BSECT) $(KHELPER)
	@for i in $(DIRS); do \
	(cd $$i; make min_rfs) ; \
	done
	cp $(ELKS_DIR)/Image $(TARGET_MNT)/boot/linux
	cp $(KHELPER) $(TARGET_MNT)/boot/boot
	umount $(TARGET_FS)
#	dd if=$(FD_BSECT) of=$(TARGET_FS) bs=512 count=2
	

clean:
	@for i in $(DIRS); do \
	(cd $$i; make clean) ; \
	done
	umount $(TARGET_FS) || true
	dd if=/dev/zero of=$(TARGET_FS) bs=1024 count=$(TARGET_BLKS)
