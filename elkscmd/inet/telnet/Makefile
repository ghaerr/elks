# Makefile for telnet

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
NET_USR_BIN	= telnet
NET_USR_LIB	= 
NET_USR_SBIN	= 

STD_BIN 	= 
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= 
STD_USR_LIB	= 
STD_USR_SBIN	= 

include $(BASEDIR)/Make.rules

###############################################################################

#LDFLAGS=-i

SRC=	ttn.c ttn_conf.c

all:	telnet

telnet:	$(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)

net_rfs: all

install: all
	cp -p telnet $(TARGET_MNT)/bin

clean:
	rm -f telnet

