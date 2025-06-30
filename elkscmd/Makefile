# Makefile for the ELKS command set.
#
###############################################################################
#
# Include standard rules.

BASEDIR = .

include $(BASEDIR)/Makefile-rules

###############################################################################
#
# Subdirectories for clean / build / install

# All subdirectories to build & clean

# unused commands but working compilations: nano levee
SUBDIRS =       \
	lib         \
	advent      \
	ash         \
	basic       \
	bc          \
	busyelks    \
	debug       \
	disk_utils  \
	fsck_dos    \
	elvis	    \
	file_utils  \
	gui         \
	inet        \
	ktcp        \
	minix1      \
	minix2      \
	minix3      \
	misc_utils  \
	sash        \
	screen      \
	cron        \
	sh_utils    \
	sys_utils   \
	tui         \
	test        \
	# EOL

ifndef CONFIG_ARCH_SOLO86
ifndef CONFIG_ARCH_SWAN
ifndef CONFIG_ARCH_8018X
SUBDIRS += nano-X
endif
endif
endif

###############################################################################
#
# Compile everything.

all:
	@if [ ! -e $(TOPDIR)/include/autoconf.h ]; \
	then echo -e "\n*** ERROR: You must configure ELKS first ***\n" >&2; exit 1; fi
	for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR all || exit 1; done
ifdef CONFIG_APP_ROMPRG
	$(MAKE) -C romprg
endif

install:
	$(MAKE) -f Make.install "CONFIG=$(TOPDIR)/.config"

clean:
	for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR clean || exit 1; done

###############################################################################
