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

# All subdirectories to build & clean

# TODO: broken command compilations
# byacc elvis screen m4 xvi
SUBDIRS =       \
	ash         \
	bc          \
	busyelks    \
	disk_utils  \
	file_utils  \
	inet        \
	ktcp        \
	levee       \
	minix1      \
	minix2      \
	minix3      \
	misc_utils  \
	mtools      \
	nano-X      \
	prems       \
	sash        \
	sh_utils    \
	sys_utils   \
	test        \
	# EOL

# Application Package List
PACKAGE=$(ELKSCMD_DIR)/Applications
# Install destination
DESTDIR=$(TOPDIR)/target
TAGS=
# Old-style installation
INSTDIRS=

#
# Install apps by source directory
#
ifdef CONFIG_APP_ASH
	TAGS += :ash|
endif

ifdef CONFIG_APP_BUSYELKS
	TAGS += :busyelks|
endif

ifdef CONFIG_APP_DISK_UTILS
	TAGS += :diskutil|
	ifeq ($(CONFIG_APP_DISK_UTILS), y)
		TAGS += :be-diskutil|
	endif
endif

ifdef CONFIG_APP_FILE_UTILS
	TAGS += :fileutil|
	ifeq ($(CONFIG_APP_FILE_UTILS), y)
		TAGS += :be-fileutil|
	endif
endif

ifdef CONFIG_APP_LEVEE
	TAGS += :levee|
endif

ifdef CONFIG_APP_KTCP
	TAGS += :net|
endif

ifdef CONFIG_APP_MINIX1
	TAGS += :minix1|
	ifeq ($(CONFIG_APP_MINIX1), y)
		TAGS += :be-minix1|
	endif
endif

ifdef CONFIG_APP_MINIX2
	TAGS += :minix2|
endif

ifdef CONFIG_APP_MINIX3
	TAGS += :minix3|
	ifeq ($(CONFIG_APP_MINIX3), y)
		TAGS += :be-minix3|
	endif
endif

ifdef CONFIG_APP_MTOOLS
	TAGS += :mtools|
endif

ifdef CONFIG_APP_MISC_UTILS
	TAGS += :miscutil|
	ifeq ($(CONFIG_APP_MISC_UTILS), y)
		TAGS += :be-miscutil|
	endif
endif

ifdef CONFIG_APP_NANOX
	TAGS += :nanox|
endif

ifdef CONFIG_APP_OTHER
	TAGS += :other|
endif

ifdef CONFIG_APP_SASH
	TAGS += :sash|
endif

ifdef CONFIG_APP_SH_UTILS
	TAGS += :shutil|
	ifeq ($(CONFIG_APP_SH_UTILS), y)
		TAGS += :be-shutil|
	endif
endif

ifdef CONFIG_APP_SYS_UTILS
	TAGS += :sysutil|
endif

ifdef CONFIG_APP_TEST
	INSTDIRS += test
endif

#
# Install apps by image size
#
ifdef CONFIG_APPS_360K
	TAGS = :boot|:360k|
endif

ifdef CONFIG_APPS_720K
	TAGS = :boot|:360k|:net|:720k|
endif

ifdef CONFIG_APPS_1440K
	TAGS = :*|
endif

ifdef CONFIG_APPS_2880K
	TAGS = :*|
endif

TAGS += :end

###############################################################################
#
# Compile everything.

include $(BASEDIR)/Make.rules

all:
	@if [ ! -e $(TOPDIR)/include/autoconf.h ]; \
	then echo -e "\n*** ERROR: You must configure ELKS first ***\n" >&2; exit 1; fi
	for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR all || exit 1; done

# use bash instead of /bin/sh for curly brace expansion
SHELL=/bin/bash
install:
	@echo "TAGS=$(TAGS)" | tr -d ' '
	awk /`echo "$(TAGS)" | tr -d ' '`/{print} $(PACKAGE) | sed '/^#/d' > Filelist
	@while IFS= read -r line; \
	do \
		file=`echo $$line | cut -d" " -f 1`; \
		dir=`echo $$line | cut -d" " -f 2`; \
		if [ "$${dir:0:2}" == "::" ]; \
		then \
			instdir=$${dir:2}; \
		else \
			instdir=bin; \
		fi; \
		echo install $$file to $$instdir; \
		if [ ! -e $$file ]; \
		then \
			echo "File specified in $(PACKAGE) not found: $$file"; \
			exit 1; \
		else \
			[ ! -d $$file ] && mkdir -p $$(dirname $(DESTDIR)/$$instdir); \
			[ -f $$file ] && $(INSTALL) $$file $(DESTDIR)/$$instdir; \
		fi \
	done < Filelist
	rm Filelist
ifdef CONFIG_APP_BUSYELKS
	$(MAKE) -C busyelks links
endif
	for DIR in $(INSTDIRS); do $(MAKE) -C $$DIR clean || exit 1; done

clean:
	for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR clean || exit 1; done

###############################################################################
