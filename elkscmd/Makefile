# Makefile for the ELKS command set.
#
###############################################################################
#
# Include standard rules.

BASEDIR = .

include $(BASEDIR)/Make.defs

###############################################################################
#
# Subdirectories for clean / build / install

# All subdirectories (for clean)

SUBDIRS =       \
	ash         \
	bc          \
	bootblocks  \
	debug_utils \
	disk_utils  \
	elvis       \
	file_utils  \
	inet        \
	ktcp        \
	levee       \
	lib         \
	minix1      \
	minix2      \
	minix3      \
	misc_utils  \
	mtools      \
	prems       \
	prn-utils   \
	rc          \
	test        \
	sash        \
	screen      \
	sh_utils    \
	sys_utils   \
	test        \
	tools       \
	xvi         \
	# EOL

# Directories to build (for all and install)

BUILDDIRS =    \
	bootblocks \
	inet       \
	test       \
	# EOL

ifeq ($(CONFIG_APP_ASH), y)
BUILDDIRS += ash
endif

ifeq ($(CONFIG_APP_BC), y)
BUILDDIRS += bc
endif

ifeq ($(CONFIG_APP_DISK_UTILS), y)
BUILDDIRS += disk_utils
endif

ifeq ($(CONFIG_APP_FILE_UTILS), y)
BUILDDIRS += file_utils
endif

ifeq ($(CONFIG_APP_LEVEE), y)
BUILDDIRS += levee
endif

ifeq ($(CONFIG_APP_KTCP), y)
BUILDDIRS += ktcp
endif

ifeq ($(CONFIG_APP_MINIX1), y)
BUILDDIRS += minix1
endif

ifeq ($(CONFIG_APP_MINIX2), y)
BUILDDIRS += minix2
endif

ifeq ($(CONFIG_APP_MINIX3), y)
BUILDDIRS += minix3
endif

ifeq ($(CONFIG_APP_MTOOLS), y)
BUILDDIRS += mtools
endif

ifeq ($(CONFIG_APP_MISC_UTILS), y)
BUILDDIRS += misc_utils
endif

ifeq ($(CONFIG_APP_PREMS), y)
BUILDDIRS += prems
endif

ifeq ($(CONFIG_APP_SASH), y)
BUILDDIRS += sash
endif

ifeq ($(CONFIG_APP_SH_UTILS), y)
BUILDDIRS += sh_utils
endif

ifeq ($(CONFIG_APP_SYS_UTILS), y)
BUILDDIRS += sys_utils
endif

###############################################################################
#
# State location of boot sector and boot helper.

FD_BSECT	= $(MINIX_BOOT)/minix.bin

KHELPER 	= $(MINIX_BOOT)/minix_elks.bin

###############################################################################
#
# Names of all possible image files.

IMAGES		= boot root comb360 comb comb_net full5 full3 full1680

# Populated items to delete for special combined 360K boot+root disk
COMB360_DEL	= sbin/partype sbin/ramdisk bin/clock

###############################################################################
#
# Compile everything.

all:
	@if [ ! -e $(TOPDIR)/include/autoconf.h ]; \
	then echo -e "\n*** ERROR: You must configure ELKS first ***\n" >&2; exit 1; fi
	+@for DIR in $(BUILDDIRS); do $(MAKE) -C $$DIR all || exit 1; done

images_clean:
	-umount -d $(IMAGES) 2>/dev/null || true
	-rm -f $(IMAGES) images.zip images.tar.*

clean:
	-umount -d $(IMAGES) || true
	-rm -f $(IMAGES) core images.zip images.tar.*
	-rm -rf $(ROOTDIR)
	-rm -f *~
	@for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR clean || exit 1; done

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################
#
# Create relevant parts of other packages.

$(FD_BSECT):
	$(MAKE) -C $(MINIX_BOOT) minix.bin

$(KHELPER):
	$(MAKE) -C $(MINIX_BOOT) minix_elks.bin

###############################################################################
#
# Create disk images

kernel_image: $(ELKS_DIR)/arch/i86/boot/Image

boot_blocks: $(FD_BSECT) $(KHELPER)
	@if [ ! -e $(MINIX_BOOT)/minix.bin ]; \
	then echo -e "\n\n *** Do builds in dev86 and dev86/bootblocks first *** \n\n"; exit 1; fi
	@if [ ! -e $(MINIX_BOOT)/minix_elks.bin ]; \
	then echo -e "\n\n *** Do builds in dev86 and dev86/bootblocks first *** \n\n"; exit 1; fi

_build_bootable_target: boot_blocks _build_target kernel_image
	mkdir -p $(TARGET_MNT)/boot
	cp $(ELKS_DIR)/arch/i86/boot/Image $(TARGET_MNT)/boot/linux
	-@cp $(KHELPER) $(TARGET_MNT)/boot/boot || \
	echo -e "\n\n *** WARNING: Failed to copy boot binary ***\n\n"
	umount -d $(TARGET_FS)
	dd if=$(FD_BSECT) of=$(TARGET_FS) bs=512 count=2 conv=notrunc 2>/dev/null

_build_nonbootable_target: _build_target
	umount -d $(TARGET_FS)

_build_target: banner check_id _populate_target

_populate_target: _mount_target
	for i in $(DIRS); do $(MAKE) -C $$i $(TARGET_RFS) || exit 1; done >/dev/null
	[ -e $(TARGET_MNT)/bin/ash ] && ln -sf ash $(TARGET_MNT)/bin/sh || \
		ln -sf sash $(TARGET_MNT)/bin/sh
	$(ELKSCMD_DIR)/tools/ver.pl $(ELKS_DIR)/Makefile-rules > $(TARGET_MNT)/etc/issue
	[ "$(COMB360)" = "yes" ] && \
	for DEL in $(COMB360_DEL); do rm $(TARGET_MNT)/$$DEL; done || true
	[ "$(INSTALL_KTCP)" = "yes" ] && \
	cp -p $(ELKSCMD_DIR)/ktcp/ktcp $(TARGET_MNT)/bin || true

_mount_target:
	@if [ "$(shell id -u)" != "0" ]; \
	then echo -e "\n\n *** Only root can build disk images *** \n\n"; exit 1; fi
	umount -d $(TARGET_FS) >/dev/null 2>&1 || true
	dd if=/dev/zero of=$(TARGET_FS) bs=1024 count=$(TARGET_BLKS) 2>/dev/null
	$(MKFS) $(MKFS_OPTS) $(TARGET_FS) >/dev/null
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

full1680:
	@if [ "$(shell id -u)" != "0" ]; \
	then echo -e "\n\n *** Only root can build disk images *** \n\n"; exit 1; fi
	@$(MAKE) _build_bootable_target \
		TARGET_FS=$(FULL1680_TARGET_FS) \
		TARGET_RFS=max_rfs \
		TARGET_BLKS=$(FULL1680_TARGET_BLKS) \
		INSTALL_KTCP=yes
full3:
	@if [ "$(shell id -u)" != "0" ]; \
	then echo -e "\n\n *** Only root can build disk images *** \n\n"; exit 1; fi
	@$(MAKE) _build_bootable_target \
		TARGET_FS=$(FULL3_TARGET_FS) \
		TARGET_RFS=max_rfs \
		TARGET_BLKS=$(FULL3_TARGET_BLKS) \
		INSTALL_KTCP=yes

full5:
	@if [ "$(shell id -u)" != "0" ]; \
	then echo -e "\n\n *** Only root can build disk images *** \n\n"; exit 1; fi
	@$(MAKE) _build_bootable_target \
		TARGET_FS=$(FULL5_TARGET_FS) \
		TARGET_RFS=rfs \
		TARGET_BLKS=$(FULL5_TARGET_BLKS) \
		INSTALL_KTCP=yes

comb360:
	@if [ "$(shell id -u)" != "0" ]; \
	then echo -e "\n\n *** Only root can build disk images *** \n\n"; exit 1; fi
	@$(MAKE) _build_bootable_target \
		TARGET_FS=$(COMB360_TARGET_FS) \
		TARGET_RFS=min_rfs \
		COMB360=yes \
		TARGET_BLKS=$(COMB360_TARGET_BLKS)

comb:
	@if [ "$(shell id -u)" != "0" ]; \
	then echo -e "\n\n *** Only root can build disk images *** \n\n"; exit 1; fi
	@$(MAKE) _build_bootable_target \
		TARGET_FS=$(COMB_TARGET_FS) \
		TARGET_RFS=min_rfs \
		TARGET_BLKS=$(COMB_TARGET_BLKS)

comb_net:
	@if [ "$(shell id -u)" != "0" ]; \
	then echo -e "\n\n *** Only root can build disk images *** \n\n"; exit 1; fi
	@$(MAKE) _build_bootable_target \
		TARGET_FS=$(COMB_NET_TARGET_FS) \
		TARGET_RFS=min_rfs \
		TARGET_BLKS=$(COMB_NET_TARGET_BLKS) \
		INSTALL_KTCP=yes

boot: $(ELKS_DIR)/arch/i86/boot/Image
	cp $(ELKS_DIR)/arch/i86/boot/Image boot
	

root:
	@if [ "$(shell id -u)" != "0" ]; \
	then echo -e "\n\n *** Only root can build disk images *** \n\n"; exit 1; fi
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

images: $(IMAGES)

images.zip: $(IMAGES)
	zip -9q images.zip $(IMAGES)
	-@stat -c "%s %n" images.zip

images.tar.gz: $(IMAGES)
	tar -c $(IMAGES) | gzip -9 > images.tar.gz
	-@stat -c "%s %n" images.tar.gz

images.tar.xz: $(IMAGES)
	tar -c $(IMAGES) | xz -e > images.tar.xz
	-@stat -c "%s %n" images.tar.xz

#######
# EOF #
#######
