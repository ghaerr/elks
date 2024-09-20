# Makefile for telnet

BASEDIR=../..

include $(BASEDIR)/Makefile-rules

###############################################################################

###############################################################################

SRCS = telnet.c ttn_conf.c
OBJS = $(SRCS:.c=.o)

all:	telnet

telnet:	$(OBJS) $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(TINYPRINTF) $(LDLIBS)

install: telnet
	$(INSTALL) telnet $(DESTDIR)/bin

clean:
	$(RM) $(OBJS) telnet
