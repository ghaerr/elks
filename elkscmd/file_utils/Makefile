BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=cat chgrp chmod chown cmp cp dd grep l ln ls mkdir mkfifo mknod \
	more mv rm rmdir sync touch 

MIN_PRGS=cat chgrp chmod chown cmp cp ls mknod mkdir mv rm rmdir \
	sync touch

SMIN_PRGS=cat cp ls mkdir sync 

all: $(PRGS)

install: $(PRGS)
	sudo install $(PRGS) $(TARGET_MNT)/bin

clean:
	rm -f $(PRGS) *~
