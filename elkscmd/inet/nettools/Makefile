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
STD_USR_SBIN	= netstat

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=netstat

SPRGS=

CFLAGS=-0 -O -ansi -I$(ELKSNET_DIR)

all: $(PRGS)

rfs:

net_rfs: install

min_rfs:

smin_rfs:

install_sibo: all
	cp -p $(SPRGS) $(TARGET_MNT)/bin

install: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

#fsck: fsck.c
#	$(CC) $(CFLAGS) $(LDFLAGS) -ansi fsck.c -o fsck -Hffff

netstat: netstat.o
	$(CC) $(LDFLAGS) netstat.o -o netstat

#.c.o:
#	$(CC) $(CFLAGS) -c $<

clean:
	rm -f core *.o $(PRGS) $(SPRGS)
