# Didn't check this file for CC and CFLAGS
VERSION = 0
PATCHLEVEL = 0
SUBLEVEL = 73
# If we're not a pre, comment the following line
PRE = "2"

.EXPORT_ALL_VARIABLES:

# linuxMT root directory
# and backup directory
#
MT_DIR     = $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
TOPDIR     = $(MT_DIR)

ARCH = i86
ARCH_DIR = arch/i86
BACKUP_DIR = /usr/local/src/elk

#
# if you want the ram-disk device, define this to be the
# size in blocks.
#
RAMDISK = -DRAMDISK=512

AS86	=as86 -0
LD86	=ld86 -0

AS	=as
LD	=ld86
LDFLAGS	=-0 -i
CC	=bcc $(RAMDISK)
# This is where I broke your work :-) MG
CFLBASE	=-D__KERNEL__ -O
CFLAGS	=$(CFLBASE) -i
CPP	=$(CC) -I$(MT_DIR)/include -E -D__KERNEL__
CC_PROTO = gcc -I$(MT_DIR)/include -M -D__KERNEL__

LINT	=lclint
CONFIG_SHELL := $(shell if [ -x "$$bash" ]; then echo $$bash; \
          else if [ -x /bin/bash ]; then echo /bin/bash; \
          else echo sh; fi ; fi)

#
# ROOT_DEV specifies the default root-device when making the image.
# This can be either FLOPPY, /dev/xxxx or empty, in which case the
# default of /dev/hd6 is used by 'build'.
#
ROOT_DEV=FLOPPY
SWAP_DEV=/dev/hd2

#########################################################################
# general construction rules

.c.s:
	$(CC) $(CFLAGS) \
	-0 -nostdinc -Iinclude -S -o $*.s $<
.s.o:
	$(AS) -0 -I$(MT_DIR)/include -c -o $*.o $<
.S.s:
	gcc -E -traditional -o $*.s $<
.c.o:
	$(CC) $(CFLAGS) \
	-0 -nostdinc -Iinclude -c -o $*.o $<

#########################################################################
# targets

all:	do-it-all

#
# Make "config" the default target if there is no configuration file or
# "depend" the target if there is no top-level dependency information.
#
ifeq (.config,$(wildcard .config))
include .config
#ifeq (.depend,$(wildcard .depend))
#include .depend
do-it-all:      Image
#else
#CONFIGURATION = dep
#do-it-all:      dep
#endif
else
CONFIGURATION = config
do-it-all:      config
endif

#
# What do we need this time?
#      
ARCHIVES=kernel/kernel.a fs/fs.a lib/lib.a net/net.a $(ARCH_DIR)/kernel/akernel.a $(ARCH_DIR)/lib/lib86.a $(ARCH_DIR)/mm/mm.a
DRIVERS =$(ARCH_DIR)/drivers/char/chr_drv.a $(ARCH_DIR)/drivers/block/blk_drv.a

# Check what filesystems to include
ifeq ($(CONFIG_MINIX_FS), y)
	ARCHIVES := $(ARCHIVES) fs/minix/minixfs.a
endif

ifeq ($(CONFIG_ROMFS_FS), y)
	ARCHIVES := $(ARCHIVES) fs/romfs/romfs.a
endif

ifeq ($(CONFIG_ELKSFS_FS), y)
	ARCHIVES := $(ARCHIVES) fs/elksfs/elksfs.a
endif

ifeq ($(CONFIG_286PMODE), y)
Image: $(ARCH_DIR)/boot/bootsect $(ARCH_DIR)/boot/setup $(ARCH_DIR)/tools/system $(ARCH_DIR)/tools/build $(ARCH_DIR)/286pmode/pmode286
	$(ARCH_DIR)/tools/build $(ARCH_DIR)/boot/bootsect $(ARCH_DIR)/boot/setup $(ARCH_DIR)/286pmode/pmode286 $(ARCH_DIR)/tools/system $(ROOT_DEV) > Image
	sync
else
Image: $(ARCH_DIR)/boot/bootsect $(ARCH_DIR)/boot/setup $(ARCH_DIR)/tools/system $(ARCH_DIR)/tools/build 
	$(ARCH_DIR)/tools/build $(ARCH_DIR)/boot/bootsect $(ARCH_DIR)/boot/setup $(ARCH_DIR)/tools/system $(ROOT_DEV) > Image
	sync
endif

boot: Image
	dd if=/dev/zero of=boot bs=1024 count=360
	dd if=Image of=boot bs=1024 conv=notrunc
	sync

disk: Image
	dd bs=8192 if=Image of=/dev/fd0

#########################################################################
# library rules
.PHONY: $(ARCH_DIR)/kernel/akernel.a $(ARCH_DIR)/lib/lib86.a \
        $(ARCH_DIR)/mm/mm.a $(ARCH_DIR)/drivers/char/chr_drv.a \
        $(ARCH_DIR)/drivers/block/blk_drv.a $(ARCH_DIR)/286pmode/pmode286 \
        fs/fs.a fs/minix/minixfs.a fs/romfs/romfs.a fs/elksfs/elksfs.a \
        kernel/kernel.a lib/lib.a net/net.a


$(ARCH_DIR)/kernel/akernel.a:
	(cd $(ARCH_DIR)/kernel; make)

$(ARCH_DIR)/lib/lib86.a:
	(cd $(ARCH_DIR)/lib; make)

$(ARCH_DIR)/mm/mm.a:
	(cd $(ARCH_DIR)/mm; make)

$(ARCH_DIR)/drivers/char/chr_drv.a:
	(cd $(ARCH_DIR)/drivers/char; make)

$(ARCH_DIR)/drivers/block/blk_drv.a:
	(cd $(ARCH_DIR)/drivers/block; make)

$(ARCH_DIR)/286pmode/pmode286:
	(cd $(ARCH_DIR)/286pmode; make)

fs/fs.a:
	(cd fs; make)

fs/minix/minixfs.a:
	(cd fs/minix; make)

fs/romfs/romfs.a:
	(cd fs/romfs; make)

fs/elksfs/elksfs.a:
	(cd fs/elksfs; make)

kernel/kernel.a: include/linuxmt/version.h include/linuxmt/compile.h
	(cd kernel; make)

lib/lib.a:
	(cd lib; make)

net/net.a:
	(cd net; make)

#########################################################################
# lint rules

lint:
	$(LINT) -I$(MT_DIR)/include -c init/main.c

#########################################################################
# arch tools

$(ARCH_DIR)/boot/setup: $(ARCH_DIR)/boot/setup.S
	gcc -E -traditional -I$(MT_DIR)/include/ -o $(ARCH_DIR)/boot/setup.s $(ARCH_DIR)/boot/setup.S
	$(AS86) -o $(ARCH_DIR)/boot/setup.o $(ARCH_DIR)/boot/setup.s
	$(LD86) -s -o $(ARCH_DIR)/boot/setup -M $(ARCH_DIR)/boot/setup.o \
	> Setup.map

$(ARCH_DIR)/boot/bootsect:	$(ARCH_DIR)/boot/bootsect.S
	gcc -E -traditional -I$(MT_DIR)/include/ -o $(ARCH_DIR)/boot/bootsect.s $(ARCH_DIR)/boot/bootsect.S
	$(AS86) -0 -o $(ARCH_DIR)/boot/bootsect.o $(ARCH_DIR)/boot/bootsect.s
	$(LD86) -0 -s -o $(ARCH_DIR)/boot/bootsect -M \
	$(ARCH_DIR)/boot/bootsect.o > Boot.map

$(ARCH_DIR)/boot/crt1.o: $(ARCH_DIR)/boot/crt1.c

$(ARCH_DIR)/boot/crt0.o: $(ARCH_DIR)/boot/crt0.s
	$(AS86) -0 -o $(ARCH_DIR)/boot/crt0.o $(ARCH_DIR)/boot/crt0.s

$(ARCH_DIR)/tools/build: $(ARCH_DIR)/tools/build.c
	gcc -I $(TOPDIR)/include -o $(ARCH_DIR)/tools/build $(ARCH_DIR)/tools/build.c

$(ARCH_DIR)/tools/system: $(ARCH_DIR)/boot/crt0.o $(ARCH_DIR)/boot/crt1.o init/main.o \
		$(ARCHIVES) $(DRIVERS) $(MATH) $(LIBS)
	$(LD) $(LDFLAGS) $(ARCH_DIR)/boot/crt0.o $(ARCH_DIR)/boot/crt1.o init/main.o \
		$(ARCHIVES) \
		$(DRIVERS) \
		$(MATH) \
		$(LIBS) \
		-t -M -o $(ARCH_DIR)/tools/system > System.map


#########################################################################
# misc

clean:
	rm -f *~ Image Boot.map Setup.map System.map tmp_make core 
	rm -f $(ARCH_DIR)/boot/bootsect $(ARCH_DIR)/boot/setup \
		  $(ARCH_DIR)/boot/bootsect.s $(ARCH_DIR)/boot/setup.s
	rm -f $(ARCH_DIR)/boot/*~ $(ARCH_DIR)/boot/*.o
	rm -f $(ARCH_DIR)/tools/system $(ARCH_DIR)/tools/build 
	rm -f $(ARCH_DIR)/tools/*~ $(ARCH_DIR)/tools/*.o
	rm -f init/*~ init/*.o 
	(cd $(ARCH_DIR)/kernel; make clean)
	(cd $(ARCH_DIR)/lib; make clean)
	(cd $(ARCH_DIR)/mm; make clean)
	(cd $(ARCH_DIR)/drivers/block; make clean)
	(cd $(ARCH_DIR)/drivers/char; make clean)
	(cd $(ARCH_DIR)/286pmode; make clean)
	(cd fs;make clean)
	(cd fs/minix;make clean)
	(cd fs/romfs;make clean)
	(cd fs/elksfs;make clean)
	(cd kernel;make clean)
	(cd lib;make clean)
	(cd net;make clean)
	(cd scripts; make clean)
	
depclean:
	@for i in `find -name Makefile`; do \
	sed '/\#\#\# Dependencies/q' < $$i > tmp_make ; \
	mv tmp_make $$i ; \
	done

distclean:	clean depclean
	rm -f .config* .menuconfig*
	rm -f scripts/lxdialog/*.o scripts/lxdialog/lxdialog

backup:	distclean
	(cd .. ; tar cf - linuxmt | compress - > $(BACKUP_DIR)/linuxMT.tar.Z)
	sync

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in init/*.c;do echo -n "init/";$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile
	(cd $(ARCH_DIR)/kernel; make dep)
	(cd $(ARCH_DIR)/lib; make dep)
	(cd $(ARCH_DIR)/mm; make dep)
	(cd $(ARCH_DIR)/drivers/block; make dep)
	(cd $(ARCH_DIR)/drivers/char; make dep)
	(cd $(ARCH_DIR)/286pmode; make dep)
	(cd fs; make dep)
	(cd fs/minix; make dep)
	(cd fs/romfs; make dep)
	(cd fs/elksfs; make dep)
	(cd kernel; make dep)
	(cd lib; make dep)
	(cd net; make dep)

# Configuration stuff
menuconfig:  
	make -C scripts/lxdialog all
	$(CONFIG_SHELL) scripts/Menuconfig arch/$(ARCH)/config.in

config:        
	$(CONFIG_SHELL) scripts/Configure arch/$(ARCH)/config.in


include/linuxmt/version.h: ./Makefile
ifdef PRE
	@echo \#define UTS_RELEASE \"$(VERSION).$(PATCHLEVEL).$(SUBLEVEL)-pre$(PRE)\" > .ver
	@echo \#define ELKS_VERSION_CODE `expr $(VERSION) \\* 65536 \\* 8 + $(PATCHLEVEL) \\* 256 \\* 8 + $(SUBLEVEL) \\* 8 + $(PRE) ` >> .ver
else
	@echo \#define UTS_RELEASE \"$(VERSION).$(PATCHLEVEL).$(SUBLEVEL)\" > .ver
	@echo \#define ELKS_VERSION_CODE `expr $(VERSION) \\* 65536 \\* 8 + $(PATCHLEVEL) \\* 256 \\* 8 + $(SUBLEVEL) \\* 8  ` >> .ver
endif
	@mv -f .ver $@

include/linuxmt/compile.h:
	@echo \#define UTS_VERSION \"\#$(VERSION) `date`\" > .ver
	@mv -f .ver $@

### Dependencies:
