BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=netstat nslookup arp

LOCALFLAGS=-I$(ELKSCMD_DIR)

all: $(PRGS)

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

netstat: netstat.o
	$(LD) $(LDFLAGS) netstat.o -o netstat $(LDLIBS)

nslookup: nslookup.o
	$(LD) $(LDFLAGS) nslookup.o -o nslookup $(LDLIBS)

arp: arp.o
	$(LD) $(LDFLAGS) arp.o -o arp $(LDLIBS)

clean:
	rm -f core *.o $(PRGS)
