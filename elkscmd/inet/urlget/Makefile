# Makefile for urlget

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
NET_USR_BIN	= urlget
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

#CFLAGS=-O 

#LDFLAGS=-i

SRC=	net.c urlget.c

all:	urlget

urlget:	$(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -I$(INC_DIR) -o urlget $(SRC)

net_rfs: all

install: all
	cp -p urlget $(TARGET_MNT)/bin

clean:
	rm -f urlget *.o

