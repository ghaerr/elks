BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

MIN_BIN 	= 
MIN_LIB 	= 
MIN_SBIN	= fdisk fsck mkfs ramdisk
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
STD_USR_SBIN	= 

include $(BASEDIR)/Make.rules

###############################################################################

PROGS=fdisk ramdisk mkfs fsck

SPROGS=mkfs

all:		$(PROGS)

rfs:		install

min_rfs:	install

net_rfs:	install

smin_rfs:	install_sibo

install_sibo:	all
	cp -p $(SPROGS) $(TARGET_MNT)/sbin

install:	all
	cp -p $(PROGS) $(TARGET_MNT)/sbin

fsck:		fsck.o

fdisk:		fdisk.o

mkfs:		mkfs.o

ramdisk:	ramdisk.o

clean:
	rm -f *.o $(FORMATMOD) core $(PROGS)
