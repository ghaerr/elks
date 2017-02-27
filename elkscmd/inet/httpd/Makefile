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

PRGS=httpd

SPRGS=

CFLAGS=-0 -O -ansi -I$(ELKSCMD_DIR) -I$(ELKS_DIR)/include

all: $(PRGS)

max_rfs: install

rfs:

net_rfs: install

min_rfs:

smin_rfs:

install_sibo: all
	cp -p $(SPRGS) $(TARGET_MNT)/bin

install: all
	cp -p $(PRGS) $(TARGET_MNT)/bin
	mkdir -p $(TARGET_MNT)/usr/lib/httpd
	cp -f -p sample_index.html $(TARGET_MNT)/usr/lib/httpd/index.html

netstat: httpd.o
	$(CC) $(CFLAGS) $(LDFLAGS) httpd.o -o httpd

clean:
	rm -f core *.o $(PRGS) $(SPRGS)
