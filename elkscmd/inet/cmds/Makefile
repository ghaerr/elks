BASEDIR=../..

include $(BASEDIR)/Makefile-rules

###############################################################################

LOCALFLAGS=-I$(ELKSCMD_DIR)

###############################################################################

PRGS = arp ftp ftpd httpd netcat netstat nslookup telnet telnetd urlget

all: $(PRGS)

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin
	$(INSTALL) -d $(DESTDIR)/var/www
	$(INSTALL) sample_index.html $(DESTDIR)/var/www/index.html
#	$(INSTALL) resolv.cfg $(DESTDIR)/etc

arp: arp.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

ftp: ftp.o
	$(LD) $(LDFLAGS) -maout-stack=6144 -maout-heap=4096 -o $@ $^ $(LDLIBS)

ftpd: ftpd.o
	$(LD) $(LDFLAGS) -maout-stack=4096 -maout-heap=4096 -o $@ $^ $(LDLIBS)

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

telnetd: telnetd.o telopt.o
	$(LD) $(LDFLAGS) -maout-heap=256 -maout-stack=1024 -o $@ $^ $(LDLIBS)

urlget: urlget.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	$(RM) *.o $(PRGS)
