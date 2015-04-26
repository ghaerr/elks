BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

MIN_BIN 	= 
MIN_LIB 	= 
MIN_SBIN	= 
MIN_USR_BIN	=
MIN_USR_LIB	= 
MIN_USR_MAN	= 
MIN_USR_SBIN	= 

NET_BIN 	= 
NET_LIB 	= 
NET_SBIN	= 
NET_USR_BIN	=
NET_USR_LIB	= 
NET_USR_MAN	= 
NET_USR_SBIN	= 

STD_BIN		= pres/pres
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= 
STD_USR_LIB	= 
STD_USR_MAN	= 
STD_USR_SBIN	= 

include $(BASEDIR)/Make.rules

###############################################################################

CFLAGS	= $(CFLBASE)

PRGS =	pres/pres

NETPRGS =

all:
	make -C prem all
	make -C pres all

max_rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

rfs: all

net_rfs: all

min_rfs: all

smin_rfs: all

clean:
	make -C pres clean
	make -C prem clean
