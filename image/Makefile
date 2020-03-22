# Makefile for the ELKS images

ifndef TOPDIR
$(error TOPDIR is not defined, ELKS not configured yet)
endif

# for CONFIG_IMG_EXTRA_IMAGES
include $(TOPDIR)/.config

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

images: images-minix hd32-minix images-fat

images-minix: fd360-minix fd720-minix fd1440-minix fd2880-minix hd32-minix

images-fat: fd360-fat fd720-fat fd1440-fat fd2880-fat

fd360-minix:
	echo CONFIG_APPS_360K=y		> Config
	echo CONFIG_IMG_FD360=y		>> Config
	echo CONFIG_IMG_MINIX=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=fd360-minix
	rm Config

fd720-minix:
	echo CONFIG_APPS_720K=y		> Config
	echo CONFIG_IMG_FD720=y		>> Config
	echo CONFIG_IMG_MINIX=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=fd720-minix
	rm Config

fd1440-minix:
	echo CONFIG_APPS_1440K=y	> Config
	echo CONFIG_IMG_FD1440=y	>> Config
	echo CONFIG_IMG_MINIX=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=fd1440-minix
	rm Config

fd2880-minix:
	echo CONFIG_APPS_2880K=y	> Config
	echo CONFIG_IMG_FD2880=y	>> Config
	echo CONFIG_IMG_MINIX=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
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
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=hd32-minix
	rm Config

fd360-fat:
	echo CONFIG_APPS_360K=y		> Config
	echo CONFIG_IMG_FD360=y		>> Config
	echo CONFIG_IMG_FAT=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=fd360-fat
	rm Config

fd720-fat:
	echo CONFIG_APPS_720K=y		> Config
	echo CONFIG_IMG_FD720=y		>> Config
	echo CONFIG_IMG_FAT=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=fd720-fat
	rm Config

fd1440-fat:
	echo CONFIG_APPS_1440K=y	> Config
	echo CONFIG_IMG_FD1440=y	>> Config
	echo CONFIG_IMG_FAT=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=fd1440-fat
	rm Config

# FAT16 image
fd2880-fat:
	echo CONFIG_APPS_2880K=y	> Config
	echo CONFIG_IMG_FD2880=y	>> Config
	echo CONFIG_IMG_FAT=y		>> Config
	echo CONFIG_IMG_DEV=y		>> Config
	echo CONFIG_IMG_BOOT=y		>> Config
	$(MAKE) -f Make.image "CONFIG=$(TOPDIR)/image/Config" NAME=fd2880-fat
	rm Config

# FAT32 image (fails on mformat currently)
hd32-fat:

# Clean target

clean:
	-rm -rf $(DESTDIR)
	-rm -f *.bin

