BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

MIN_BIN 	= 
MIN_LIB 	= 
MIN_SBIN	= 
MIN_USR_BIN	= cut uniq wc
MIN_USR_LIB	= 
MIN_USR_SBIN	= 

NET_BIN 	= 
NET_LIB 	= 
NET_SBIN	= 
NET_USR_BIN	= decomp16
NET_USR_LIB	= 
NET_USR_SBIN	= 

STD_BIN 	= 
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= banner cksum du fgrep grep proto sum
STD_USR_LIB	= 
STD_USR_SBIN	= 

include $(BASEDIR)/Make.rules

###############################################################################

CFLAGS	= $(CFLBASE) -D_POSIX_SOURCE

xCC=gcc
xCFLAGS=-D_POSIX_SOURCE -D_PROTOTYPE\(a,b\)=

PRGS =	banner cksum cut decomp16 du fgrep grep proto sum uniq wc

NETPRGS = decomp16 du grep wc

all: $(PRGS)

rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

net_rfs: all
	cp -p $(NETPRGS) $(TARGET_MNT)/bin

min_rfs: all

smin_rfs: all

clean:
	rm -f $(PRGS)
