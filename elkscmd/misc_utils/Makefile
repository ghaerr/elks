BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

HOSTCC = gcc
HOST_CFLAGS = -O2

###############################################################################

OUTS = tar miniterm unreal16
OUTS_BE = ed

#TODO: fix compress uncompress zcat lpfilter disabled

PRGS = $(OUTS)
ifeq "$(CONFIG_APP_MISC_UTILS)" "y"
	PRGS += $(OUTS_BE)
endif

PRGS_HOST=compress.host

all: $(PRGS) $(PRGS_HOST)

ed: ed.o
	$(LD) $(LDFLAGS) -o ed ed.o $(LDLIBS)

tar: tar.o
	$(LD) $(LDFLAGS) -o tar tar.o $(LDLIBS)
	
unreal16: unreal16
	$(LD) -melks-libc -mcmodel=small -c unreal16.S -o unreal16.o
	$(LD) -melks-libc -mcmodel=small -nostdlib -o unreal16 unreal16.o
	
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
	$(INSTALL) $(PRGS) $(DESTDIR)/usr/bin

clean:
	rm -f $(OUTS) $(OUTS_BE) $(PRGS_HOST) *~ *.o
