include Make.defs

DIRS=disk_utils file_utils sh_utils sys_utils misc_utils sash \
	levee ash minix1 minix2 bc byacc elvis inet

KHELPER=$(MINIX_BOOT)/minix_elks.bin
FD_BSECT=$(MINIX_BOOT)/minix.bin

all:
	@for i in $(DIRS); do make -C $$i ; done

$(COMB_TARGET_FS):
	dd if=/dev/zero of=$(COMB_TARGET_FS) bs=1024 count=$(COMB_TARGET_BLKS)

$(ROOT_TARGET_FS):
	dd if=/dev/zero of=$(ROOT_TARGET_FS) bs=1024 count=$(ROOT_TARGET_BLKS)

$(ROOT_NET_TARGET_FS):
	dd if=/dev/zero of=$(ROOT_NET_TARGET_FS) bs=1024 count=$(ROOT_NET_TARGET_BLKS)

$(SIBO_TARGET_FS):
	dd if=/dev/zero of=$(SIBO_TARGET_FS) bs=1024 count=$(SIBO_TARGET_BLKS)

$(ELKSCMD_DIR)/rootfs_template/etc/passwd: rootfs_template.tar
	tar xpf rootfs_template.tar

$(ELKSCMD_DIR)/rootfs_template/etc/issue: $(ELKSCMD_DIR)/rootfs_template/etc/passwd $(ELKS_DIR)/Makefile
	$(ELKSCMD_DIR)/tools/ver.pl $(ELKS_DIR)/Makefile > $(ELKSCMD_DIR)/rootfs_template/etc/issue

mount_combfs: $(COMB_TARGET_FS) $(ELKSCMD_DIR)/rootfs_template/etc/issue
	$(MKFS) $(MKFS_OPTS) $(COMB_TARGET_FS) $(COMB_TARGET_BLKS)
	mount $(COMB_TARGET_FS) $(TARGET_MNT) $(LOOP)
	(cd $(ELKSCMD_DIR)/rootfs_template ; tar cf - --exclude CVS *) | \
		(cd $(TARGET_MNT) ; tar xpvf -)
	(cd $(TARGET_MNT)/dev ; ./MAKEDEV)

mount_rootfs: $(ROOT_TARGET_FS) $(ELKSCMD_DIR)/rootfs_template/etc/issue
	$(MKFS) $(MKFS_OPTS) $(ROOT_TARGET_FS) $(ROOT_TARGET_BLKS)
	mount $(ROOT_TARGET_FS) $(TARGET_MNT) $(LOOP)
	(cd $(ELKSCMD_DIR)/rootfs_template ; tar cf - --exclude CVS *) | \
		(cd $(TARGET_MNT) ; tar xpvf -)
	(cd $(TARGET_MNT)/dev ; ./MAKEDEV)

mount_rootnetfs: $(ROOT_NET_TARGET_FS) $(ELKSCMD_DIR)/rootfs_template/etc/issue
	$(MKFS) $(MKFS_OPTS) $(ROOT_NET_TARGET_FS) $(ROOT_NET_TARGET_BLKS)
	mount $(ROOT_NET_TARGET_FS) $(TARGET_MNT) $(LOOP)
	(cd $(ELKSCMD_DIR)/rootfs_template ; tar cf - --exclude CVS *) | \
		(cd $(TARGET_MNT) ; tar xpvf -)
	(cd $(TARGET_MNT)/dev ; ./MAKEDEV)
	
mount_sibofs: $(SIBO_TARGET_FS) $(ELKSCMD_DIR)/rootfs_template/etc/issue
	$(MKFS) $(MKFS_OPTS) $(SIBO_TARGET_FS) $(SIBO_TARGET_BLKS)
	mount $(SIBO_TARGET_FS) $(TARGET_MNT) $(LOOP)
	(cd $(ELKSCMD_DIR)/rootfs_template ; tar cf - --exclude CVS *) | \
		(cd $(TARGET_MNT) ; tar xpvf -)
	(cd $(TARGET_MNT) ; rm -rf home boot root var)
	cp SIBODEV $(TARGET_MNT)/dev/SIBODEV
	(cd $(TARGET_MNT)/dev ; ./SIBODEV ; rm -f MAKEDEV SIBODEV )
	

#rfs: mount_rfs
#	@for i in $(DIRS); do make -C $$i rfs ; done
#	umount $(TARGET_FS)
	
#min_rfs: mount_rfs
#	@for i in $(DIRS); do make -C $$i min_rfs ; done
#	umount $(TARGET_FS)

$(ELKS_DIR)/.config:
	make -C $(ELKS_DIR) config

$(ELKS_DIR)/arch/i86/boot/Image: $(ELKS_DIR)/.config
	make -C $(ELKS_DIR)

$(FD_BSECT):
	make -C $(MINIX_BOOT) bin

$(KHELPER):
	make -C $(MINIX_BOOT)

boot: $(ELKS_DIR)/arch/i86/boot/Image
	cp $(ELKS_DIR)/arch/i86/boot/Image boot
	

comb: mount_combfs $(ELKS_DIR)/arch/i86/boot/Image $(FD_BSECT) $(KHELPER)
	@for i in $(DIRS); do make -C $$i min_rfs ; done
	cp $(ELKS_DIR)/arch/i86/boot/Image $(TARGET_MNT)/boot/linux
	cp $(KHELPER) $(TARGET_MNT)/boot/boot
	umount $(COMB_TARGET_FS)
	dd if=$(FD_BSECT) of=$(COMB_TARGET_FS) bs=512 count=2 conv=notrunc

root: mount_rootfs
	@for i in $(DIRS); do make -C $$i min_rfs ; done
	umount $(ROOT_TARGET_FS)
	
comb_net: mount_rootnetfs $(ELKS_DIR)/arch/i86/boot/Image $(FD_BSECT) $(KHELPER)
	cd $(ELKSNET_DIR)/ktcp ; \
	make ; \
	cp -p -f ktcp $(TARGET_MNT)/bin
	@for i in $(DIRS); do make -C $$i net_rfs ; done
	cp $(ELKS_DIR)/arch/i86/boot/Image $(TARGET_MNT)/boot/linux
	cp $(KHELPER) $(TARGET_MNT)/boot/boot
	umount $(ROOT_NET_TARGET_FS)
	dd if=$(FD_BSECT) of=$(ROOT_NET_TARGET_FS) bs=512 count=2 conv=notrunc

sibo: mount_sibofs
	@for i in $(DIRS); do make -C $$i smin_rfs ; done
	umount $(SIBO_TARGET_FS)

images.zip: boot comb root comb_net
	zip images.zip boot root comb comb_net

clean:
	@for i in $(DIRS); do make -C $$i clean ; done
	umount $(COMB_TARGET_FS) || true
	umount $(ROOT_TARGET_FS) || true
	umount $(ROOT_NET_TARGET_FS) || true
	rm -f $(ROOT_TARGET_FS) $(COMB_TARGET_FS) $(SIBO_TARGET_FS) $(ROOT_NET_TARGET_FS)
	rm -f boot images.zip
#	rm -rf rootfs_template
