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

# unused commands but working compilations: nano-X nano levee
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
	ekermit	    \
	elvis	    \
	file_utils  \
	gui         \
	inet        \
	ktcp        \
	minix1      \
	minix2      \
	minix3      \
	misc_utils  \
	romprg      \
	sash        \
	screen      \
	cron        \
	sh_utils    \
	sys_utils   \
	tui         \
	test        \
	# EOL

OWCDIRS =       \
    basic       \
    fsck_dos    \
    gui         \
    # EOL

C86DIRS =       \
    gui         \
    # EOL

###############################################################################
#
# Compile all IA16 apps, OpenWatcom apps if CONFIG_APPS_WATCOM set, and C86 apps same

ALL = ia16
ifdef CONFIG_APPS_WATCOM
ALL += owc
endif
ifdef CONFIG_APPS_C86
ALL += c86
endif

all: $(ALL)

ia16:
	@if [ ! -e $(TOPDIR)/include/autoconf.h ]; \
	then echo -e "\n*** ERROR: You must configure ELKS first ***\n" >&2; exit 1; fi
	for DIR in $(IA16DIRS); do $(MAKE) -C $$DIR all || exit 1; done

owc:
	for DIR in $(OWCDIRS); do $(MAKE) -C $$DIR -f Makefile.owc all || exit 1; done

c86:
	for DIR in $(C86DIRS); do $(MAKE) -C $$DIR -f Makefile.c86 all || exit 1; done

clean: owclean c86clean
	for DIR in $(IA16DIRS); do $(MAKE) -C $$DIR clean || exit 1; done

owclean:
	for DIR in $(OWCDIRS); do $(MAKE) -C $$DIR -f Makefile.owc clean || exit 1; done

c86clean:
	for DIR in $(C86DIRS); do $(MAKE) -C $$DIR -f Makefile.c86 clean || exit 1; done

install:
	$(MAKE) -f Make.install "CONFIG=$(TOPDIR)/.config"

###############################################################################
