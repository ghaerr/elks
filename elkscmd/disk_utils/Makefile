BASEDIR=..

include $(BASEDIR)/Make.defs

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
