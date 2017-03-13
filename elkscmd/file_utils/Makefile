BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

CFLAGS = $(CFLBASE)

PRGS=cat chgrp chmod chown cmp cp dd grep l ln ls mkdir mkfifo mknod \
	more mv rm rmdir sync touch 

MIN_PRGS=cat chgrp chmod chown cmp cp ls mknod mkdir mv rm rmdir \
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
