BASEDIR=../..

include $(BASEDIR)/Makefile-rules

###############################################################################

LOCALFLAGS=-I$(ELKSCMD_DIR)

###############################################################################

PRGS=netstat nslookup arp

all: $(PRGS)

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin
#	$(INSTALL) resolv.cfg $(DESTDIR)/etc

netstat: netstat.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) netstat.o $(TINYPRINTF) -o netstat $(LDLIBS)

nslookup: nslookup.o
	$(LD) $(LDFLAGS) nslookup.o -o nslookup $(LDLIBS)

arp: arp.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) arp.o $(TINYPRINTF) -o arp $(LDLIBS)

clean:
	$(RM) *.o $(PRGS)
