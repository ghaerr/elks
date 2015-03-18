BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

MIN_BIN 	= 
MIN_LIB 	= 
MIN_SBIN	= 
MIN_USR_BIN	= cat chmod cmp cp ln ls mkdir mv rm rmdir sync touch 
MIN_USR_LIB	= 
MIN_USR_MAN	= 
MIN_USR_SBIN	= chgrp chown mknod

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
STD_USR_BIN	= dd grep more
STD_USR_LIB	= 
STD_USR_MAN	= 
STD_USR_SBIN	= mkfifo

include $(BASEDIR)/Make.rules

###############################################################################

CFLAGS = $(CFLBASE)

PRGS=cat chgrp chmod chown cmp cp dd grep l ln ls mkdir mkfifo mknod \
	more mv rm rmdir sync touch 

MIN_PRGS=cat chgrp chmod chown cmp cp l ls mknod mkdir mv rm rmdir \
	sync touch

SMIN_PRGS=cat cp ls mkdir sync 

all: $(PRGS)

max_rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

min_rfs: all
	cp -p $(MIN_PRGS) $(TARGET_MNT)/bin

net_rfs: min_rfs

smin_rfs: all
	cp -p $(SMIN_PRGS) $(TARGET_MNT)/bin

clean:
	rm -f $(PRGS) *~
