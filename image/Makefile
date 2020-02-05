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
VERBOSE=-v

# Determine what to build for 'all' target

TARGETS=

ifdef CONFIG_IMG_BOOT
TARGETS += $(FD_BSECT) $(ELKS_DIR)/arch/i86/boot/Image
endif

TARGETS += template

ifdef CONFIG_IMG_MINIX
TARGETS += minixfs
endif

ifdef CONFIG_IMG_ROM
TARGETS += romfs
endif

.PHONY: all $(TARGETS)

all: $(TARGETS)

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

# Create MINIX filesystem from template

minixfs: template
	mfs $(VERBOSE) $(TARGET_FILE) genfs $(MFS_OPTS) $(DESTDIR)
ifdef CONFIG_IMG_LINK
	# command to make char/block devices in image (no sudo required)
	$(MAKE) -f Make.devices "MKDEV=mfs $(TARGET_FILE) mknod"
endif
ifdef CONFIG_IMG_BOOT
	mfs $(TARGET_FILE) boot $(FD_BSECT)
endif
	mfsck -fv $(TARGET_FILE)
	mfs $(TARGET_FILE) stat

# Create ROM filesystem from template

romfs: template
	-rm -f romfs.devices
	$(MAKE) -f Make.devices "MKDEV=echo >> romfs.devices"
	mkromfs -d romfs.devices $(DESTDIR)

# Clean target
clean:
	-rm -f $(TARGET_FILE)
	-rm -rf $(DESTDIR)
	-rm -f *.bin

# Application package management
# Type "make images" after normal ELKS build to create all MINIX disk image sets
# Applications are tagged for disk images in the 'Packages' file
allimages: images dosimages

images: fd360 fd720 fd1440

dosimages: fd360-msdos fd720-msdos fd1440-msdos

fd360:
	$(MAKE) -f Make.package minix NAME=fd360 OPTS="-i192 -s360" BPB=-B9,2 APPS=":boot|:small"

fd720:
	$(MAKE) -f Make.package minix NAME=fd720 OPTS="-i256 -s720" BPB=-B9,2 APPS=":boot|:small|:base|:net"

fd1440:
	$(MAKE) -f Make.package minix NAME=fd1440 OPTS="-i360 -s1440" BPB=-B18,2 APPS=":boot|:small|:base|:net|:shutil|:fileutil|:app|:nanox"

fd360-msdos:
	$(MAKE) -f Make.package msdos NAME=fd360-msdos SIZE=360 APPS=":boot|:small"

fd720-msdos:
	$(MAKE) -f Make.package msdos NAME=fd720-msdos SIZE=720 APPS=":boot|:small|:base|:net"

fd1440-msdos:
	$(MAKE) -f Make.package msdos NAME=fd1440-msdos SIZE=1440 APPS=":boot|:small|:base|:net|:shutil|:fileutil|:app|:nanox"
