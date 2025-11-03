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
IA16DIRS =       \
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
IA16DIRS += nano-X
endif
endif
endif

OWCDIRS =       \
    basic       \
    fsck_dos    \
    gui         \
    # EOL

###############################################################################
#
# Compile all IA16 apps, and OpenWatcom apps if CONFIG_APPS_OWC set

ALL = ia16
ifdef CONFIG_APPS_OWC
ALL += owc
endif

all: $(ALL)

ia16:
	@if [ ! -e $(TOPDIR)/include/autoconf.h ]; \
	then echo -e "\n*** ERROR: You must configure ELKS first ***\n" >&2; exit 1; fi
	for DIR in $(IA16DIRS); do $(MAKE) -C $$DIR all || exit 1; done
ifdef CONFIG_APP_ROMPRG
	$(MAKE) -C romprg
endif

owc:
	for DIR in $(OWCDIRS); do $(MAKE) -C $$DIR -f Makefile.owc all || exit 1; done

install:
	$(MAKE) -f Make.install "CONFIG=$(TOPDIR)/.config"

clean: owclean
	for DIR in $(IA16DIRS); do $(MAKE) -C $$DIR clean || exit 1; done

owclean:
	for DIR in $(OWCDIRS); do $(MAKE) -C $$DIR -f Makefile.owc clean || exit 1; done

###############################################################################
