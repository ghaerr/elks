include ../Make.defs

PROGS=fdisk ramdisk mkfs fsck

SPROGS=mkfs

all: $(PROGS)

rfs: install

min_rfs: install

net_rfs: install

smin_rfs: install_sibo

install_sibo: all
	cp -p $(SPROGS) $(TARGET_MNT)/bin

install: all
	cp -p $(PROGS) $(TARGET_MNT)/bin

fsck: fsck.c
	$(CC) $(CFLAGS) $(LDFLAGS) -ansi fsck.c -o fsck -Hffff

fdisk: fdisk.o
	$(CC) $(LDFLAGS) fdisk.o -o fdisk


.c.o:
#	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(FORMATMOD) core $(PROGS)
