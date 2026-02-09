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
# Compile all IA16 apps

ALL = ia16

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
