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
# TODO: restore elvis, prn-utils, screen & xvi

SUBDIRS =       \
	ash         \
	bc          \
	bootblocks  \
	busyelks    \
	debug_utils \
	disk_utils  \
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
	nano-X      \
	prems       \
	test        \
	sash        \
	sh_utils    \
	sys_utils   \
	test        \
	tools       \
	# EOL

# Directories to install

INSTDIRS = \
	inet \
	# EOL

ifeq ($(CONFIG_APP_ASH), y)
INSTDIRS += ash
endif

ifeq ($(CONFIG_APP_BC), y)
INSTDIRS += bc
endif

ifeq ($(CONFIG_APP_BUSYELKS), y)
INSTDIRS += busyelks
endif

ifdef CONFIG_APP_DISK_UTILS
INSTDIRS += disk_utils
endif

ifdef CONFIG_APP_FILE_UTILS
INSTDIRS += file_utils
endif

ifeq ($(CONFIG_APP_LEVEE), y)
INSTDIRS += levee
endif

ifeq ($(CONFIG_APP_KTCP), y)
INSTDIRS += ktcp
endif

ifdef CONFIG_APP_MINIX1
INSTDIRS += minix1
endif

ifeq ($(CONFIG_APP_MINIX2), y)
INSTDIRS += minix2
endif

ifdef CONFIG_APP_MINIX3
INSTDIRS += minix3
endif

ifeq ($(CONFIG_APP_MTOOLS), y)
INSTDIRS += mtools
endif

ifdef CONFIG_APP_MISC_UTILS
INSTDIRS += misc_utils
endif

ifeq ($(CONFIG_APP_NANOX), y)
INSTDIRS += nano-X
endif

ifeq ($(CONFIG_APP_PREMS), y)
INSTDIRS += prems
endif

ifeq ($(CONFIG_APP_SASH), y)
INSTDIRS += sash
endif

ifeq ($(CONFIG_APP_SH_UTILS), y)
INSTDIRS += sh_utils
endif

ifeq ($(CONFIG_APP_SYS_UTILS), y)
INSTDIRS += sys_utils
endif

ifdef CONFIG_APP_TEST
INSTDIRS += test
endif

###############################################################################
#
# Compile everything.

include $(BASEDIR)/Make.rules

all:
	@if [ ! -e $(TOPDIR)/include/autoconf.h ]; \
	then echo -e "\n*** ERROR: You must configure ELKS first ***\n" >&2; exit 1; fi
	for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR all || exit 1; done

install:
	for i in $(INSTDIRS); do $(MAKE) -C $$i install || exit 1; done

clean:
	for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR clean || exit 1; done

###############################################################################
