# Makefile for the ELKS images

include Make.defs
include Make.rules

# Template directory for installing applications and linux before genfs
TEMPLATE_DIR = $(ELKSCMD_DIR)/rootfs_template

# DESTDIR is used in elkscmd/* Makefiles for installation destination
DESTDIR = $(TOPDIR)/target
export DESTDIR

# add image block size to mkfs options for mfs
MFS_OPTS=$(MKFS_OPTS) -s$(TARGET_BLKS) -k
#VERBOSE=-v

#
# Determine what to build for All target
TARGETS=
ifdef CONFIG_IMG_BOOT
TARGETS += $(FD_BSECT) $(ELKS_DIR)/arch/i86/boot/Image
endif
TARGETS += template image
ifdef CONFIG_IMG_ROM
TARGETS += romfs
endif

all: $(TARGETS)
	mfsck -fv $(TARGET_FILE)
	mfs $(TARGET_FILE) stat

#
# Create character and block devices in image (make target devices:)
include Make.devices

#
# Populate template for filesystem genfs
template:
	-rm -rf $(DESTDIR)
	cp -a $(TEMPLATE_DIR) $(DESTDIR)
	find $(DESTDIR) -name .keep -delete
	$(MAKE) -C $(ELKSCMD_DIR) install
	bash -c "$(ELKSCMD_DIR)/tools/ver.pl $(ELKS_DIR)/Makefile-rules > $(DESTDIR)/etc/issue"
ifdef CONFIG_IMG_BOOT
	install $(ELKS_DIR)/arch/i86/boot/Image $(DESTDIR)/linux
endif
	find $(DESTDIR) \
		-type d ! -name root -execdir chmod go+rx '{}' ';' -o \
		! -type d ! -name .profile -execdir chmod go+r '{}' ';'

#
# Create MINIX filesystem from template
image:
	mfs $(VERBOSE) $(TARGET_FILE) genfs $(MFS_OPTS) $(DESTDIR)
ifdef CONFIG_IMG_LINK
	# command to make char/block devices in image (no sudo required)
	$(MAKE) -f Make.devices "MKDEV=mfs $(TARGET_FILE) mknod"
endif
ifdef CONFIG_IMG_BOOT
	mfs $(TARGET_FILE) boot $(FD_BSECT)
endif

romfs:
	-rm -f romfs.devices
	$(MAKE) -f Make.devices "MKDEV=echo >> romfs.devices"
	mkromfs -d romfs.devices $(DESTDIR)

# Clean target
clean:
	-rm -f $(TARGET_FILE)
	-rm -rf $(DESTDIR)
	-rm -f *.bin
