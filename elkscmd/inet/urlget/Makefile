# Makefile for urlget

BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

SRCS = net.c urlget.c
OBJS = $(SRCS:.c=.o)

all:	urlget

urlget:	$(OBJS) $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o urlget $(OBJS) $(TINYPRINTF) $(LDLIBS)

install: urlget
	$(INSTALL) urlget $(DESTDIR)/bin

clean:
	rm -f urlget *.o

