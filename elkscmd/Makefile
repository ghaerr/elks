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

ifdef CONFIG_APP_TEST
BUILDDIRS += test
endif

###############################################################################
#
# Compile everything.

include $(BASEDIR)/Make.rules

all:
	@if [ ! -e $(TOPDIR)/include/autoconf.h ]; \
	then echo -e "\n*** ERROR: You must configure ELKS first ***\n" >&2; exit 1; fi
	for DIR in $(BUILDDIRS); do $(MAKE) -C $$DIR all || exit 1; done

install:
	for i in $(BUILDDIRS); do $(MAKE) -C $$i install || exit 1; done

clean:
	for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR clean || exit 1; done

###############################################################################
