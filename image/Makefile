# Makefile for the ELKS images

ifndef TOPDIR
$(error TOPDIR is not defined, ELKS not configured yet)
endif

# for CONFIG_IMG_EXTRA_IMAGES
include $(TOPDIR)/.config

# MBR boot sector
HD_MBR_BOOT = $(TOPDIR)/bootblocks/mbr.bin

# Directory for final filesystem to be generated from
DESTDIR = $(TOPDIR)/target
export DESTDIR

TARGETS = image

ifdef CONFIG_IMG_EXTRA_IMAGES
TARGETS += images
endif

.PHONY: all clean

all: $(TARGETS)

image:
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/.config"

copy: copyminix

copyminix:
	$(MAKE) -f Make.image copyminix "CONFIG=$(TOPDIR)/.config"

copyfat:
	$(MAKE) -f Make.image copyfat "CONFIG=$(TOPDIR)/.config"

copyrom:
	$(MAKE) -f Make.image copyrom "CONFIG=$(TOPDIR)/.config"

compress:
	cd $(TOPDIR)/target/bin; elks-compress *

images: images-minix images-hd images-fat

images-minix: fd360-minix fd720-minix fd1200-minix fd1440-minix fd2880-minix

images-fat: fd360-fat fd720-fat fd1200-fat fd1440-fat fd2880-fat hd32-fat hd32mbr-fat

images-hd: hd32-minix hd32mbr-minix hd64-minix

fd360-minix:
	echo CONFIG_APPS_360K=y		> Config
	echo CONFIG_IMG_FD360=y		>> Config
	echo CONFIG_IMG_MINIX=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	sed -n -e '/CONFIG_TIME_/p'	        >> Config < $(TOPDIR)/.config
	sed -n -e '/CONFIG_APPS_COMPRESS/p'	>> Config < $(TOPDIR)/.config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=fd360-minix
	rm Config

fd720-minix:
	echo CONFIG_APPS_720K=y		> Config
	echo CONFIG_IMG_FD720=y		>> Config
	echo CONFIG_IMG_MINIX=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	sed -n -e '/CONFIG_TIME_/p'	        >> Config < $(TOPDIR)/.config
	sed -n -e '/CONFIG_APPS_COMPRESS/p'	>> Config < $(TOPDIR)/.config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=fd720-minix
	rm Config

fd1200-minix:
	echo CONFIG_APPS_1200K=y	> Config
	echo CONFIG_IMG_FD1200=y	>> Config
	echo CONFIG_IMG_MINIX=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	sed -n -e '/CONFIG_TIME_/p'	        >> Config < $(TOPDIR)/.config
	sed -n -e '/CONFIG_APPS_COMPRESS/p'	>> Config < $(TOPDIR)/.config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=fd1200-minix
	rm Config

fd1440-minix:
	echo CONFIG_APPS_1440K=y	> Config
	echo CONFIG_IMG_FD1440=y	>> Config
	echo CONFIG_IMG_MINIX=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	sed -n -e '/CONFIG_TIME_/p'	        >> Config < $(TOPDIR)/.config
	sed -n -e '/CONFIG_APPS_COMPRESS/p'	>> Config < $(TOPDIR)/.config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=fd1440-minix
	rm Config

fd2880-minix:
	echo CONFIG_APPS_2880K=y	> Config
	echo CONFIG_IMG_FD2880=y	>> Config
	echo CONFIG_IMG_MINIX=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	sed -n -e '/CONFIG_TIME_/p'	>> Config < $(TOPDIR)/.config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=fd2880-minix
	rm Config

hd32-minix:
	echo CONFIG_APPS_2880K=y	> Config
	echo CONFIG_IMG_HD=y		>> Config
	echo CONFIG_IMG_BLOCKS=31752 >> Config
	echo CONFIG_IMG_SECT=63		>> Config
	echo CONFIG_IMG_HEAD=16		>> Config
	echo CONFIG_IMG_CYL=63		>> Config
	echo CONFIG_IMG_MINIX=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	sed -n -e '/CONFIG_TIME_/p'	>> Config < $(TOPDIR)/.config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=hd32-minix
	rm Config

hd64-minix:
	echo CONFIG_APPS_2880K=y	> Config
	echo CONFIG_IMG_HD=y		>> Config
	echo CONFIG_IMG_BLOCKS=65535 >> Config
	echo CONFIG_IMG_SECT=63		>> Config
	echo CONFIG_IMG_HEAD=16		>> Config
	echo CONFIG_IMG_CYL=127		>> Config
	echo CONFIG_IMG_MINIX=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	sed -n -e '/CONFIG_TIME_/p'	>> Config < $(TOPDIR)/.config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=hd64-minix
	rm Config

fd360-fat:
	echo CONFIG_APPS_360K=y		> Config
	echo CONFIG_IMG_FD360=y		>> Config
	echo CONFIG_IMG_FAT=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	sed -n -e '/CONFIG_TIME_/p'	        >> Config < $(TOPDIR)/.config
	sed -n -e '/CONFIG_APPS_COMPRESS/p'	>> Config < $(TOPDIR)/.config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=fd360-fat
	rm Config

fd720-fat:
	echo CONFIG_APPS_720K=y		> Config
	echo CONFIG_IMG_FD720=y		>> Config
	echo CONFIG_IMG_FAT=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	sed -n -e '/CONFIG_TIME_/p'	        >> Config < $(TOPDIR)/.config
	sed -n -e '/CONFIG_APPS_COMPRESS/p'	>> Config < $(TOPDIR)/.config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=fd720-fat
	rm Config

fd1200-fat:
	echo CONFIG_APPS_1200K=y	> Config
	echo CONFIG_IMG_FD1200=y	>> Config
	echo CONFIG_IMG_FAT=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	sed -n -e '/CONFIG_TIME_/p'	        >> Config < $(TOPDIR)/.config
	sed -n -e '/CONFIG_APPS_COMPRESS/p'	>> Config < $(TOPDIR)/.config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=fd1200-fat
	rm Config

fd1440-fat:
	echo CONFIG_APPS_1440K=y	> Config
	echo CONFIG_IMG_FD1440=y	>> Config
	echo CONFIG_IMG_FAT=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	sed -n -e '/CONFIG_TIME_/p'	        >> Config < $(TOPDIR)/.config
	sed -n -e '/CONFIG_APPS_COMPRESS/p'	>> Config < $(TOPDIR)/.config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=fd1440-fat
	rm Config

# FAT16 image
fd2880-fat:
	echo CONFIG_APPS_2880K=y	> Config
	echo CONFIG_IMG_FD2880=y	>> Config
	echo CONFIG_IMG_FAT=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	sed -n -e '/CONFIG_TIME_/p'	>> Config < $(TOPDIR)/.config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=fd2880-fat
	rm Config

# FAT32 image
hd32-fat:
	echo CONFIG_APPS_2880K=y	> Config
	echo CONFIG_IMG_HD=y		>> Config
	echo CONFIG_IMG_BLOCKS=31752 >> Config
	echo CONFIG_IMG_SECT=63		>> Config
	echo CONFIG_IMG_HEAD=16		>> Config
	echo CONFIG_IMG_CYL=63		>> Config
	echo CONFIG_IMG_FAT=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	sed -n -e '/CONFIG_TIME_/p'	>> Config < $(TOPDIR)/.config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=hd32-fat
	rm Config

# MBR images
hd32mbr-minix:
	dd if=/dev/zero bs=512 count=63 | cat - hd32-minix.img > hd32mbr-minix.img
	setboot hd32mbr-minix.img -P63,16,63 -Sm $(HD_MBR_BOOT)

hd32mbr-fat:
	dd if=/dev/zero bs=512 count=63 | cat - hd32-fat.img > hd32mbr-fat.img
	setboot hd32mbr-fat.img -P63,16,63 -Sf $(HD_MBR_BOOT)

# Clean target

clean:
	-rm -rf $(DESTDIR)
	-rm -f *.img *.bin
