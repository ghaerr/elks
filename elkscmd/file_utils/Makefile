include ../Make.defs

PRGS=cat chgrp chmod chown cmp cp dd grep l ln ls mkdir mkfifo mknod \
	more mv rm rmdir sync touch 
MIN_PRGS=cat chgrp chmod chown cmp cp l ls mkdir mv rm rmdir sync touch

all: $(PRGS)

rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

min_rfs: all
	cp -p $(MIN_PRGS) $(TARGET_MNT)/bin

clean:
	rm -f $(PRGS)
