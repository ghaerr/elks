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

# TODO: broken command compilations: byacc m4 xvi
# unused commands but working compilations: mtools nano prems
SUBDIRS =       \
	lib         \
	advent      \
	ash         \
	basic       \
	bc          \
	busyelks    \
	disk_utils  \
	fsck_dos    \
	elvis	    \
	file_utils  \
	inet        \
	ktcp        \
	levee       \
	minix1      \
	minix2      \
	minix3      \
	misc_utils  \
	nano-X      \
	sash        \
	screen      \
	cron        \
	sh_utils    \
	sys_utils   \
	tui         \
	test        \
	# EOL

###############################################################################
#
# Compile everything.

include $(BASEDIR)/Make.rules

all:
	@if [ ! -e $(TOPDIR)/include/autoconf.h ]; \
	then echo -e "\n*** ERROR: You must configure ELKS first ***\n" >&2; exit 1; fi
	for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR all || exit 1; done
ifdef CONFIG_APP_CGATEXT)
	$(MAKE) -C cgatext
endif
ifdef CONFIG_APP_ROMPRG
	$(MAKE) -C romprg
endif

install:
	$(MAKE) -f Make.install "CONFIG=$(TOPDIR)/.config"

clean:
	for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR clean || exit 1; done

###############################################################################
