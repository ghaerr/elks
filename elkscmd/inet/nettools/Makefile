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

CFLAGS=-0 -O -ansi -I$(ELKSNET_DIR) -I$(ELKS_DIR)/include

all: $(PRGS)

rfs:

net_rfs: install

min_rfs:

smin_rfs:

install_sibo: all
	cp -p $(SPRGS) $(TARGET_MNT)/bin

install: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

netstat: netstat.o
	$(CC) $(CFLAGS) $(LDFLAGS) netstat.o -o netstat

clean:
	rm -f core *.o $(PRGS) $(SPRGS)
