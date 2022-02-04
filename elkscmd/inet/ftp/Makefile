# Makefile for ftp client and server

BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

SRCS = ftpd.c ftp.c
BINS = ftp ftpd
OBJS = $(SRCS:.c=.o)

all:	$(BINS)

ftp:	ftp.o
	$(LD) $(LDFLAGS) -maout-stack=6144 -maout-heap=4096 -o ftp ftp.o $(LDLIBS)

ftpd:	ftpd.o
	$(LD) $(LDFLAGS) -maout-stack=4096 -maout-heap=4096 -o ftpd ftpd.o $(LDLIBS)

install: ftpd ftp
	$(INSTALL) $(BINS)  $(DESTDIR)/bin

clean:
	rm -f $(BINS) $(OBJS)

