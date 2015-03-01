BASEDIR=..

LOCALFLAGS=-D_POSIX_SOURCE

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
NET_USR_BIN	= lp remsync synctree tget
NET_USR_LIB	= 
NET_USR_MAN	= 
NET_USR_SBIN	= lpd

STD_BIN 	= 
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= env pwdauth
STD_USR_LIB	= 
STD_USR_MAN	= 
STD_USR_SBIN	= install mt

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=env install lp pwdauth remsync synctree tget

# lpd mt	# Do not compile.

NETPRGS=env

all: $(PRGS)

lpd: lpd.o ../lib/mktemp.o

../lib/mktemp.o:
	make -C ../lib mktemp.o

max_rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

net_rfs: all
	cp -p $(NETPRGS) $(TARGET_MNT)/bin

min_rfs: all

smin_rfs: all

clean:
	rm -f core *.o $(PRGS)
