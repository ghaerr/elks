include ../Make.defs

PRGS=fdisk ramdisk mkfs fsck

SPRGS=mkfs

all: $(PRGS)

rfs: install

min_rfs: install

smin_rfs: install_sibo

install_sibo: all
	cp -p $(SPRGS) $(TARGET_MNT)/bin

install: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

fsck: fsck.c
	$(CC) $(CFLAGS) $(LDFLAGS) -ansi fsck.c -o fsck -Hffff

fdisk: fdisk.o
	$(CC) $(LDFLAGS) fdisk.o -o fdisk


.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(FORMATMOD) fdisk ramdisk core mkfs fsck
