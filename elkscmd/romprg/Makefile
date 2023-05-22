BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

CFLAGS += -Wno-maybe-uninitialized

###############################################################################

PRGS = romprg
OBJS = main.o flash.o commands.o crc.o protocol.o serial.o
#MAPFILE = -Wl,-Map=romprWg.map

#HOSTPRGS = hostromprg

all: $(PRGS) $(HOSTPRGS)

romprg: $(OBJS) commands.h crc.h serial.h version.h
	$(LD) $(LDFLAGS) $(MAPFILE) -o romprg $(OBJS) $(LDLIBS)

HOSTSRC = pc-main.c protocol.c pc-serial.c pc-commands.c crc.c
hostromprg: $(HOSTSRC) pc-serial.h pc-commands.h crc.h
	$(HOSTCC) $(HOSTCFLAGS) $(HOSTSRC) -o $@

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	rm -f $(PRGS) $(HOSTPRGS) *.o *.map



