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

STD_BIN 	= 
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= ed tar compress uncompress zcat
STD_USR_LIB	= 
STD_USR_MAN	= compress.1 uncompress.1 zcat.1
STD_USR_SBIN	= 

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=ed tar compress uncompress zcat

all: $(PRGS)

compress:
	$(CC) $(CFLAGS) -X-H0xe000 -s $@.c -o $@

uncompress: compress
	ln -sf compress uncompress

zcat: compress
	ln -sf compress zcat

rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

net_rfs: rfs

min_rfs: rfs

smin_rfs:

clean:
	rm -f core $(PRGS)

