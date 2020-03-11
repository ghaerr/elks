BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

HOSTCC = gcc
HOST_CFLAGS = -O2

###############################################################################

OUTS = tar miniterm
OUTS_BE = ed

#TODO: fix compress uncompress zcat lpfilter disabled

ifdef CONFIG_APP_MISC_UTILS
	PRGS = $(OUTS)
 ifeq "$(CONFIG_APP_MISC_UTILS)" "y"
	PRGS += $(OUTS_BE)
 endif
endif

PRGS_HOST=compress.host

all: $(PRGS) $(PRGS_HOST)

ed: ed.o
	$(LD) $(LDFLAGS) -o ed ed.o $(LDLIBS)

tar: tar.o
	$(LD) $(LDFLAGS) -o tar tar.o $(LDLIBS)
	
compress: compress.o
	$(LD) $(LDFLAGS) -o compress compress.o $(LDLIBS)

miniterm: miniterm.o
	$(LD) $(LDFLAGS) -o miniterm miniterm.o $(LDLIBS)

compress.host: compress.c
	$(HOSTCC) $(HOST_CFLAGS) -s compress.c -o $@

uncompress: compress
	ln -sf compress uncompress

zcat: compress
	ln -sf compress zcat

install: $(PRGS)
	if test "$(PROGS)" != ''; then $(INSTALL) $(PRGS) $(DESTDIR)/bin; fi

clean:
	rm -f $(OUTS) $(OUTS_BE) $(PRGS_HOST) *~ *.o
