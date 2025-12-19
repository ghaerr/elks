BASEDIR=../..

include $(BASEDIR)/Makefile-rules

###############################################################################

LOCALFLAGS=-I$(ELKSCMD_DIR)

###############################################################################

PRGS = arp httpd netcat netstat nslookup telnet urlget

all: $(PRGS)

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin
	$(INSTALL) -d $(DESTDIR)/var/www
	$(INSTALL) sample_index.html $(DESTDIR)/var/www/index.html
#	$(INSTALL) resolv.cfg $(DESTDIR)/etc

arp: arp.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

httpd: httpd.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -maout-heap=256 -maout-stack=2048 -o $@ $^ $(LDLIBS)

netcat: netcat.o
	$(LD) $(LDFLAGS) -maout-heap=128 -maout-stack=1024 -o $@ $^ $(LDLIBS)

netstat: netstat.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

nslookup: nslookup.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

telnet: telnet.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

urlget: urlget.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	$(RM) *.o $(PRGS)
