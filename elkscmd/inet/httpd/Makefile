# Makefile for httpd

BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

MIN_BIN 	= 
MIN_LIB 	= 
MIN_SBIN	= 
MIN_USR_BIN	= 
MIN_USR_LIB	= 
MIN_USR_SBIN	= 

NET_BIN 	= 
NET_LIB 	= 
NET_SBIN	= 
NET_USR_BIN	= 
NET_USR_LIB	= 
NET_USR_SBIN	= 

STD_BIN 	= 
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= 
STD_USR_LIB	= 
STD_USR_SBIN	= httpd

include $(BASEDIR)/Make.rules

###############################################################################

SHELL = /bin/sh

#CFLAGS = -c -O

CFILES = httpd.c

OBJ = $(CFILES:.c=.o)

all: httpd

#.c.o:
#	$(CC) $(CFLAGS) -o $@ $<

httpd: $(OBJ)
	$(CC) $(OPT) -o httpd $(OBJ) $(BASELIBS)

net_rfs: all

install: all
	mkdir $(TARGET_MNT)/usr/lib/httpd
	cp -f -p sample_index.html $(TARGET_MNT)/usr/lib/httpd/index.html
	cp -p httpd $(TARGET_MNT)/bin

clean: 
	rm -f core httpd $(OBJ)

dep:
	makedepend $(CFILES)
