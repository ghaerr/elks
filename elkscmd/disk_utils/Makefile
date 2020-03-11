BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

OUTS = ramdisk mkfs fsck partype
OUTS_BE = fdisk

PROGS = $(OUTS)
ifeq "$(CONFIG_APP_DISK_UTILS)" "y"
	PROGS += $(OUTS_BE)
endif

SPROGS=mkfs

all: $(PROGS)

install_sibo: all
	$(INSTALL) $(SPROGS) $(DESTDIR)/bin

install: all
	$(INSTALL) $(PROGS) $(DESTDIR)/bin

fsck: fsck.o
	$(LD) $(LDFLAGS) -o fsck fsck.o $(LDLIBS)

fdisk: fdisk.o
	$(LD) $(LDFLAGS) -o fdisk fdisk.o $(LDLIBS)

mkfs: mkfs.o
	$(LD) $(LDFLAGS) -o mkfs mkfs.o $(LDLIBS)

partype: partype.o
	$(LD) $(LDFLAGS) -o partype partype.o $(LDLIBS)

ramdisk: ramdisk.o
	$(LD) $(LDFLAGS) -o ramdisk ramdisk.o $(LDLIBS)

clean:
	rm -f *.o $(FORMATMOD) core $(OUTS) $(OUTS_BE)
