BASEDIR=..

LOCALFLAGS=

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

STD_BIN 	= sed
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= cal diff file find head sort tail tee
STD_USR_LIB	= 
STD_USR_MAN	= 
STD_USR_SBIN	=

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=cal diff file find head sed sort tail tee

NETPRGS=

all: $(PRGS)

rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

net_rfs: all
	cp -p $(NETPRGS) $(TARGET_MNT)/bin

min_rfs: all

smin_rfs: all

clean:
	rm -f core *.o $(PRGS)
