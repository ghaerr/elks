BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

CFLAGS += -Wno-maybe-uninitialized

###############################################################################

PRGS = beep
OBJS = beep.o
#MAPFILE = -Wl,-Map=romprWg.map

#HOSTPRGS = hostromprg

all: $(PRGS) $(HOSTPRGS)

beep: $(OBJS)
	$(LD) $(LDFLAGS) $(MAPFILE) -o beep $(OBJS) $(LDLIBS)

HOSTSRC = beep.c
hostromprg: $(HOSTSRC)
	$(HOSTCC) $(HOSTCFLAGS) $(HOSTSRC) -o $@

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	rm -f $(PRGS) $(HOSTPRGS) *.o *.map



