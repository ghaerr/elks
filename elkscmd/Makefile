# Makefile for the ELKS command set.
#
###############################################################################
#
# Include standard rules.

BASEDIR = .

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
		  m4 minix1 minix2 minix3 misc_utils mtools sash sh_utils \
		  inet

DONTUSE 	= debug_utils lib prn-utils rc tools xvi

DONTWORK	= elvis

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
	umount $(COMB_TARGET_FS) $(ROOT_TARGET_FS) $(ROOT_NET_TARGET_FS) \
		$(FULL3_TARGET_FS) $(FULL5_TARGET_FS) || true
	rm -f $(COMB_TARGET_FS) $(ROOT_TARGET_FS) $(ROOT_NET_TARGET_FS) \
		$(FULL3_TARGET_FS) $(FULL5_TARGET_FS) boot images.zip core
	rm -rf $(ROOTDIR)
	@for i in $(DIRS); do make -C $$i clean ; done

###############################################################################
#
# Include standard packaging commands.

MIN_BIN 	= 
MIN_LIB 	= 
MIN_SBIN	= 
MIN_USR_BIN	= 
MIN_USR_LIB	= 
MIN_USR_MAN	= 
MIN_USR_SBIN	= 

NET_BIN 	= 
NET_LIB 	= 
NET_SBIN	= 
NET_USR_BIN	= 
NET_USR_LIB	= 
NET_USR_MAN	= 
NET_USR_SBIN	= 

STD_BIN 	= 
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= 
STD_USR_LIB	= 
STD_USR_MAN	= 
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
	make -C $(MINIX_BOOT)
	make -C $(MINIX_BOOT) bootbin

$(KHELPER):
	make -C $(MINIX_BOOT) minix_elks.bin

###############################################################################
#
# Create disk images

kernel_image: $(ELKS_DIR)/arch/i86/boot/Image

boot_blocks: $(FD_BSECT) $(KHELPER)

_build_bootable_target: _build_target kernel_image boot_blocks
	mkdir -p $(TARGET_MNT)/boot
	cp $(ELKS_DIR)/arch/i86/boot/Image $(TARGET_MNT)/boot/linux
	cp $(KHELPER) $(TARGET_MNT)/boot/boot
	umount $(TARGET_FS)
	dd if=$(FD_BSECT) of=$(TARGET_FS) bs=512 count=2 conv=notrunc 2>/dev/null

_build_nonbootable_target: _build_target
	umount $(TARGET_FS)

_build_target: all banner check_id _populate_target

_populate_target: _mount_target
	for i in $(DIRS); do make -C $$i $(TARGET_RFS)||exit 1; done >/dev/null
	$(ELKSCMD_DIR)/tools/ver.pl $(ELKS_DIR)/Makefile > $(TARGET_MNT)/etc/issue
	[ "$(INSTALL_KTCP)" = "yes" ] && \
	cp -p $(ELKSNET_DIR)/ktcp/ktcp $(TARGET_MNT)/bin || true

_mount_target:
	umount $(TARGET_FS) >/dev/null 2>&1 || true
	dd if=/dev/zero of=$(TARGET_FS) bs=1024 count=$(TARGET_BLKS) 2>/dev/null
	$(MKFS) $(MKFS_OPTS) $(TARGET_FS) $(TARGET_BLKS) >/dev/null
	mkdir -p $(TARGET_MNT)
	mount $(LOOP) $(TARGET_FS) $(TARGET_MNT)
	(cd $(ELKSCMD_DIR)/rootfs_template; \
	tar cf - --exclude CVS --exclude .keep *) | \
	(cd $(TARGET_MNT); tar xpf -)
	@(cd $(TARGET_MNT)/dev; ./MAKEDEV; rm MAKEDEV MAKESET)

banner:
	@echo "*"
	@echo "* Building image: `basename $(TARGET_FS)`"
	@echo "*"

check_id:
	@if [ "`id -u`" -ne "0" ]; then \
		echo -e "\n\nERROR: Only root can build disk images.\n\n"; \
	exit 1; \
	fi

full3:
	@$(MAKE) _build_bootable_target \
		TARGET_FS=$(FULL3_TARGET_FS) \
		TARGET_RFS=rfs \
		TARGET_BLKS=$(FULL3_TARGET_BLKS) \
		INSTALL_KTCP=yes

full5:
	@$(MAKE) _build_bootable_target \
		TARGET_FS=$(FULL5_TARGET_FS) \
		TARGET_RFS=rfs \
		TARGET_BLKS=$(FULL5_TARGET_BLKS) \
		INSTALL_KTCP=yes

comb:
	@$(MAKE) _build_bootable_target \
		TARGET_FS=$(COMB_TARGET_FS) \
		TARGET_RFS=min_rfs \
		TARGET_BLKS=$(COMB_TARGET_BLKS)

comb_net:
	@$(MAKE) _build_bootable_target \
		TARGET_FS=$(ROOT_NET_TARGET_FS) \
		TARGET_RFS=net_rfs \
		TARGET_BLKS=$(ROOT_NET_TARGET_BLKS) \
		INSTALL_KTCP=yes

boot: $(ELKS_DIR)/arch/i86/boot/Image
	cp $(ELKS_DIR)/arch/i86/boot/Image boot
	

root:
	@$(MAKE) _build_nonbootable_target \
		TARGET_FS=$(ROOT_TARGET_FS) \
		TARGET_RFS=min_rfs \
		TARGET_BLKS=$(ROOT_TARGET_BLKS)
       
# FIXME: should use SIBODEV instead of MAKEDEV
sibo:
	@$(MAKE) _build_nonbootable_target \
		TARGET_FS=$(SIBO_TARGET_FS) \
		TARGET_RFS=smin_rfs \
		TARGET_BLKS=$(SIBO_TARGET_BLKS)

images.zip: boot comb root comb_net full3 full5
	zip images.zip boot root comb comb_net full3 full5

#######
# EOF #
#######
