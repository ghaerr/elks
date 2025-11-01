# Makefile for the ELKS command set.

include Makefile-rules

###############################################################################
#
# Compile everything.

all:
	@if [ ! -e $(TOPDIR)/include/autoconf.h ]; \
	then echo -e "\n*** ERROR: You must configure ELKS first ***\n" >&2; exit 1; fi
	for DIR in `sed -e '/^#/d' -e 's/\/.*//' Applications | sort | uniq`; do $(MAKE) -C $$DIR all || exit 1; done
ifdef CONFIG_APP_ROMPRG
	$(MAKE) -C romprg
endif

install:
	$(MAKE) -f Make.install "CONFIG=$(TOPDIR)/.config"

clean:
	for DIR in `sed -e '/^#/d' -e 's/\/.*//' Applications | sort | uniq`; do $(MAKE) -C $$DIR clean || exit 1; done

###############################################################################
