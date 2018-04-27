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

ifdef $(CONFIG_APP_TEST)
BUILDDIRS += test
endif

###############################################################################
#
# State location of boot sector and boot helper.

FD_BSECT	= $(MINIX_BOOT)/minix.bin

KHELPER 	= $(MINIX_BOOT)/minix_elks.bin

###############################################################################
#
# Names of all possible image files.

IMAGES		= fd360 fd720 fd1440 fd1680

###############################################################################
#
# Compile everything.

all:
	@if [ ! -e $(TOPDIR)/include/autoconf.h ]; \
	then echo -e "\n*** ERROR: You must configure ELKS first ***\n" >&2; exit 1; fi
	+@for DIR in $(BUILDDIRS); do $(MAKE) -C $$DIR all || exit 1; done

images_clean:
	-umount -d $(IMAGES) || true
	-rm -f $(IMAGES) images.zip images.tar.*

clean:
	-sudo umount -d $(TARGET_MNT) || true
	-rm -f $(IMAGES) core images.zip images.tar.*
	-rm -f *~
	@for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR clean || exit 1; done

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################
#
# Create disk images

kernel_image: $(ELKS_DIR)/arch/i86/boot/Image

boot_blocks: $(FD_BSECT) $(KHELPER)

ifdef CONFIG_IMG_BOOT
image: boot_blocks kernel_image
else
image:
endif
	mkdir -p $(IMGDIR)
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
		tar cf - --exclude CVS --exclude .keep *) | \
		(cd $(TARGET_MNT); sudo tar xpf - --no-same-owner)
	(cd $(TARGET_MNT)/dev; sudo ./MAKEDEV; sudo rm MAKEDEV MAKESET)
	for i in $(BUILDDIRS); do $(MAKE) -C $$i install || exit 1; done
	sudo bash -c "$(ELKSCMD_DIR)/tools/ver.pl $(ELKS_DIR)/Makefile-rules > $(TARGET_MNT)/etc/issue"
ifdef CONFIG_IMG_BOOT
	sudo mkdir -p $(TARGET_MNT)/boot
	sudo install $(ELKS_DIR)/arch/i86/boot/Image $(TARGET_MNT)/boot/linux
	sudo install $(KHELPER) $(TARGET_MNT)/boot/boot
endif
ifdef CONFIG_IMG_DEV
	sudo umount -d $(TARGET_FILE)
ifdef CONFIG_IMG_BOOT
	dd if=$(FD_BSECT) of=$(TARGET_FILE) bs=512 count=2 conv=notrunc
endif
endif
ifdef CONFIG_IMG_ROM
	cd $(IMGDIR) && $(ELKS_DIR)/tools/mkromfs $(TARGET_MNT)
endif

# These ones are now broken
# Should be moved into a global script

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
