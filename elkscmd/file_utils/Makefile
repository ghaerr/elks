include ../Make.defs

CFLAGS = $(CFLBASE)

PRGS=cat chgrp chmod chown cmp cp dd grep ln ls mkdir mkfifo mknod \
	more mv rm rmdir sync touch 
MIN_PRGS=cat chgrp chmod chown cmp cp l ls mknod mkdir mv rm rmdir sync touch
SMIN_PRGS=cat cp ls mkdir sync 

all: $(PRGS)

rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

min_rfs: all
	cp -p $(MIN_PRGS) $(TARGET_MNT)/bin

smin_rfs: all
	cp -p $(SMIN_PRGS) $(TARGET_MNT)/bin

clean:
	rm -f $(PRGS)
