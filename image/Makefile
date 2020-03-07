# Makefile for the ELKS images

include Make.defs
include Make.rules

# Template directory for installing applications and linux before mkfs

TEMPLATE_DIR = $(ELKSCMD_DIR)/rootfs_template

# DESTDIR is used in elkscmd/* Makefiles for installation destination

DESTDIR = $(TOPDIR)/target
export DESTDIR

#VERBOSE=-v

# Determine what to build for 'all' target

TARGETS =

ifdef CONFIG_IMG_BOOT
TARGETS += $(FD_MINIX_BOOT) $(FD_FAT_BOOT) $(ELKS_DIR)/arch/i86/boot/Image
endif

ifdef CONFIG_IMG_MINIX
TARGETS += template minixfs
endif

ifdef CONFIG_IMG_FAT
TARGETS += template fatfs
endif

ifdef CONFIG_IMG_ROM
TARGETS += template romfs
endif

ifdef CONFIG_IMG_RAW
TARGETS += raw
endif

.PHONY: all $(TARGETS)

all: $(TARGETS)

# Populate template for filesystem genfs

template:
	-rm -rf $(DESTDIR)
	cp -a $(TEMPLATE_DIR) $(DESTDIR)
	find $(DESTDIR) -name .keep -delete
	$(MAKE) -C $(ELKSCMD_DIR) install
	bash -c "./ver.pl $(ELKS_DIR)/Makefile-rules > $(DESTDIR)/etc/issue"
	git log --abbrev-commit | head -1 | sed 's/commit/ELKS built from commit/' > $(DESTDIR)/etc/motd
ifdef CONFIG_IMG_BOOT
	install $(ELKS_DIR)/arch/i86/boot/Image $(DESTDIR)/linux
endif

# Create MINIX filesystem from template

minixfs: template
	rm -f $(TARGET_FILE)
	mfs $(VERBOSE) $(TARGET_FILE) genfs $(MKFS_OPTS) $(DESTDIR)
ifdef CONFIG_IMG_DEV
	# command to make char/block devices in image (no sudo required)
	$(MAKE) -f Make.devices "MKDEV=mfs $(TARGET_FILE) mknod"
endif
ifdef CONFIG_IMG_BOOT
	setboot $(TARGET_FILE) $(FD_MINIX_BOOT)
endif
	mfsck -fv $(TARGET_FILE)
	mfs $(TARGET_FILE) stat

# Create FAT filesystem from template

fatfs: template
	rm -f $(TARGET_FILE)
	dd if=/dev/zero of=$(TARGET_FILE) bs=1024 count=$(TARGET_BLKS)
	mformat -i $(TARGET_FILE) $(MKFS_OPTS)
	# Linux has to be the first file for the boot sector loader
	rm -f linux; touch linux
	mcopy -i $(TARGET_FILE) $(CPFS_OPTS) linux ::/linux
	rm linux
	# Device folder has to be first or second for the 'fake dev' to work
	mmd -i $(TARGET_FILE) ::/dev
	for f in $$(cd $(DESTDIR); find * -name '*'); do \
		[ -d $(DESTDIR)/$$f -a "$$f" != "dev" ] && mmd -i $(TARGET_FILE) ::$$f; \
		[ -f $(DESTDIR)/$$f ] && mcopy -i $(TARGET_FILE) $(CPFS_OPTS) $(DESTDIR)/$$f ::$$f; \
	done
	# Protect contiguous /linux by marking as RO, System and Hidden
	mattrib -i $(TARGET_FILE) +r +s +h ::/linux
	# Read boot sector, skip FAT BPB, set ELKS PB sectors/heads and write boot
ifdef CONFIG_IMG_BOOT
	setboot $(TARGET_FILE) -F $(FD_FAT_BOOT)
endif

# Create ROM filesystem from template

romfs: template
	-rm -f romfs.devices
	$(MAKE) -f Make.devices "MKDEV=echo >> romfs.devices"
	mkromfs -d romfs.devices $(DESTDIR)

# Create RAW filesystem

raw: $(ELKS_DIR)/arch/i86/boot/Image
	dd if=/dev/zero of=$(TARGET_FILE) bs=1024 count=$(TARGET_BLKS)
	dd if=$(ELKS_DIR)/arch/i86/boot/Image of=$(TARGET_FILE) conv=notrunc

# Clean target

clean:
	-rm -f $(TARGET_FILE)
	-rm -rf $(DESTDIR)
	-rm -f *.bin

# Application package management

# Type 'make images' after normal ELKS build to create all disk images
# Applications are tagged for disk images in the 'Packages' file

images: images-minix images-fat


images-minix: fd360-minix fd720-minix fd1440-minix

fd360-minix:
	$(MAKE) -f Make.package NAME=fd360 FS=minix SIZE=360 TAGS=":boot|:small"

fd720-minix:
	$(MAKE) -f Make.package NAME=fd720 FS=minix SIZE=720 TAGS=":boot|:small|:base|:net"

fd1440-minix:
	$(MAKE) -f Make.package NAME=fd1440 FS=minix SIZE=1440 TAGS=":boot|:small|:base|:net|:shutil|:fileutil|:app|:nanox"


images-fat: fd360-fat fd720-fat fd1440-fat fd2880-fat

fd360-fat:
	$(MAKE) -f Make.package NAME=fd360 FS=fat SIZE=360 TAGS=":boot|:small"

fd720-fat:
	$(MAKE) -f Make.package NAME=fd720 FS=fat SIZE=720 TAGS=":boot|:small|:base|:net"

fd1440-fat:
	$(MAKE) -f Make.package NAME=fd1440 FS=fat SIZE=1440 TAGS=":boot|:small|:base|:net|:shutil|:fileutil|:app|:nanox"

# FAT16 image
fd2880-fat:
	$(MAKE) -f Make.package NAME=fd2880 FS=fat SIZE=2880 TAGS=":boot|:small|:base|:net|:shutil|:fileutil|:app|:nanox"
