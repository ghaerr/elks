# Makefile for the ELKS command set.
#
###############################################################################
#
# Include standard rules.

BASEDIR=.

include $(BASEDIR)/Make.defs

###############################################################################
#
# State directories to compile, as follows:
#
#	DIRS		Directories to compile automatically.
#
#	DONTUSE 	Directories compiled as part of other directories.
#
#	DONTWORK	Directories that don't currently compile.
#

DIRS		= sys_utils ash bc byacc disk_utils file_utils levee \
		  minix1 minix2 misc_utils sash sh_utils inet

DONTWORK	= elvis

DONTUSE 	= debug_utils lib m4 mtools prn-utils rc tools xvi

###############################################################################
#
# State location of boot sector and boot helper.

FD_BSECT	= $(MINIX_BOOT)/minix.bin

KHELPER 	= $(MINIX_BOOT)/minix_elks.bin

###############################################################################
#
# Compile everything.

all:
	@for i in $(DIRS); do make -C $$i all ; done

clean:
	@for i in $(DIRS); do make -C $$i clean ; done
	umount $(COMB_TARGET_FS) $(ROOT_TARGET_FS) $(ROOT_NET_TARGET_FS) \
		|| true
	rm -f $(ROOT_TARGET_FS) $(COMB_TARGET_FS) $(SIBO_TARGET_FS) \
		$(ROOT_NET_TARGET_FS) $(ROOTDIR) boot images.zip
	rm -rf rootfs_template
	@tar xf rootfs_template.tar

###############################################################################
#
# Include standard packaging commands.

MIN_BIN 	= 
MIN_LIB 	= 
MIN_SBIN	= 
MIN_USR_BIN	= 
MIN_USR_LIB	= 
MIN_USR_SBIN	= 

NET_BIN 	= 
NET_LIB 	= 
NET_SBIN	= 
NET_USR_BIN	= 
NET_USR_LIB	= 
NET_USR_SBIN	= 

STD_BIN 	= 
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= 
STD_USR_LIB	= 
STD_USR_SBIN	= 

include $(BASEDIR)/Make.rules

###############################################################################
#
# Create standard packages.

basepkg:
	rm -fr $(ROOTDIR)
	mkdir $(ROOTDIR)
	make mkbase

netpkg:
	rm -fr $(ROOTDIR)
	mkdir $(ROOTDIR)
	make mknet

stdpkg:
	rm -fr $(ROOTDIR)
	mkdir $(ROOTDIR)
	make mkstd

maxpkg:
	rm -fr $(ROOTDIR)
	mkdir $(ROOTDIR)
	make mkmax

###############################################################################
#
# Create relevant parts of ELKS package.

$(ELKS_DIR)/.config:
	make -C $(ELKS_DIR) config

$(ELKS_DIR)/arch/i86/boot/Image: $(ELKS_DIR)/.config
	make -C $(ELKS_DIR)

###############################################################################
#
# Create relevant parts of other packages.

$(FD_BSECT):
	make -C $(MINIX_BOOT) bin

$(KHELPER):
	make -C $(MINIX_BOOT)

###############################################################################
#
# Old commands, left as is for now.

$(COMB_TARGET_FS):
	dd if=/dev/zero of=$(COMB_TARGET_FS) bs=1024 count=$(COMB_TARGET_BLKS)

$(FULL3_TARGET_FS):
	dd if=/dev/zero of=$(FULL3_TARGET_FS) bs=1024 count=$(FULL3_TARGET_BLKS)

$(FULL5_TARGET_FS):
	dd if=/dev/zero of=$(FULL5_TARGET_FS) bs=1024 count=$(FULL5_TARGET_BLKS)

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

mount_full3fs: $(FULL3_TARGET_FS) $(ELKSCMD_DIR)/rootfs_template/etc/issue
	$(MKFS) $(MKFS_OPTS) $(FULL3_TARGET_FS) $(FULL3_TARGET_BLKS)
	mount $(FULL3_TARGET_FS) $(TARGET_MNT) $(LOOP)
	(cd $(ELKSCMD_DIR)/rootfs_template ; tar cf - --exclude CVS *) | \
		(cd $(TARGET_MNT) ; tar xpvf -)
	(cd $(TARGET_MNT)/dev ; ./MAKEDEV)

mount_full5fs: $(FULL5_TARGET_FS) $(ELKSCMD_DIR)/rootfs_template/etc/issue
	$(MKFS) $(MKFS_OPTS) $(FULL5_TARGET_FS) $(FULL5_TARGET_BLKS)
	mount $(FULL5_TARGET_FS) $(TARGET_MNT) $(LOOP)
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

boot: $(ELKS_DIR)/arch/i86/boot/Image
	cp $(ELKS_DIR)/arch/i86/boot/Image boot
	

comb: mount_combfs $(ELKS_DIR)/arch/i86/boot/Image $(FD_BSECT) $(KHELPER)
	@for i in $(DIRS); do make -C $$i min_rfs ; done
	cp $(ELKS_DIR)/arch/i86/boot/Image $(TARGET_MNT)/boot/linux
	cp $(KHELPER) $(TARGET_MNT)/boot/boot
	umount $(COMB_TARGET_FS)
	dd if=$(FD_BSECT) of=$(COMB_TARGET_FS) bs=512 count=2 conv=notrunc

full3: mount_full3fs $(ELKS_DIR)/arch/i86/boot/Image $(FD_BSECT) $(KHELPER)
	@for i in $(DIRS); do make -C $$i min_rfs ; done
	cp $(ELKS_DIR)/arch/i86/boot/Image $(TARGET_MNT)/boot/linux
	cp $(KHELPER) $(TARGET_MNT)/boot/boot
	umount $(FULL3_TARGET_FS)
	dd if=$(FD_BSECT) of=$(FULL3_TARGET_FS) bs=512 count=2 conv=notrunc

full5: mount_full3fs $(ELKS_DIR)/arch/i86/boot/Image $(FD_BSECT) $(KHELPER)
	@for i in $(DIRS); do make -C $$i min_rfs ; done
	cp $(ELKS_DIR)/arch/i86/boot/Image $(TARGET_MNT)/boot/linux
	cp $(KHELPER) $(TARGET_MNT)/boot/boot
	umount $(FULL3_TARGET_FS)
	dd if=$(FD_BSECT) of=$(FULL3_TARGET_FS) bs=512 count=2 conv=notrunc

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

images.zip: boot comb root comb_net full3 full5
	zip images.zip boot root comb comb_net full3 full5

#######
# EOF #
#######
