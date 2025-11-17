# Makefile for telnet

BASEDIR=../..

include $(BASEDIR)/Makefile-rules

###############################################################################

###############################################################################

OBJS = telnet.o

all:	telnet

telnet:	$(OBJS) $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(TINYPRINTF) $(LDLIBS)

install: telnet
	$(INSTALL) telnet $(DESTDIR)/bin

clean:
	$(RM) $(OBJS) telnet
