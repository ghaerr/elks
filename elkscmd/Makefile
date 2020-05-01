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
# byacc screen m4 xvi
SUBDIRS =       \
	ash         \
	bc          \
	busyelks    \
	disk_utils  \
	elvis	    \
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
	screen      \
	sh_utils    \
	sys_utils   \
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
	make -C cgatext
endif

install:
	$(MAKE) -f Make.install "CONFIG=$(TOPDIR)/.config"

clean:
	for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR clean || exit 1; done

###############################################################################
