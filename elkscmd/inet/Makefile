BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

MIN_BIN 	= 
MIN_LIB 	= 
MIN_SBIN	= 
MIN_USR_BIN	= 
MIN_USR_LIB 	= 
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
STD_USR_SBIN	= 

include $(BASEDIR)/Make.rules

###############################################################################

DIRS=nettools httpd telnet urlget

DIRS2=

all:
	@for i in $(DIRS); do make -C $$i all ; done

rfs:

net_rfs:
	@for i in $(DIRS); do make -C $$i net_rfs install ; done

min_rfs:

smin_rfs:

install: all
	@for i in $(DIRS); do make -C $$i install ; done

clean:
	@for i in $(DIRS) $(DIRS2); do make -C $$i clean ; done
