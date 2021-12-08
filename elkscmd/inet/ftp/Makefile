# Makefile for ftp client and server

BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

SRCS = ftpd.c
OBJS = $(SRCS:.c=.o)

all:	ftpd

ftpd:	$(OBJS)
	$(LD) $(LDFLAGS) -o ftpd $(OBJS) $(LDLIBS)

install: ftpd
	$(INSTALL) ftpd $(DESTDIR)/bin

clean:
	rm -f ftpd *.o

