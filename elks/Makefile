VERSION 	= 0
PATCHLEVEL	= 0
SUBLEVEL	= 85
PRE		= 2		# If we're not a pre, comment this line

#########################################################################
# Variables derived directly from the ELKS version number

DISTDIR 	= elks-$(VSN)
ifne (X$PRE, X)
VSN		= $(VERSION).$(PATCHLEVEL).$(SUBLEVEL)-pre$(PRE)
else
VSN		= $(VERSION).$(PATCHLEVEL).$(SUBLEVEL)
endif

.EXPORT_ALL_VARIABLES:

#########################################################################
# Relative path to ELKS base directory

ELKSDIR 	= .

#########################################################################
# linuxMT root and backup directories

MT_DIR		= $(shell cd "$ELKSDIR" ; \
			  if [ "$$PWD" != "" ]; then echo $$PWD; \
						else pwd; fi)
TOPDIR		= $(MT_DIR)

TARGET_NB_IMAGE = /tftpboot/elksy/nbImage

#########################################################################
# Specify architecture

ARCH		= i86
ARCH_DIR	= arch/$(ARCH)

#########################################################################
# Specify standard programs and flags

AS		= as86

CC		= bcc
CFLBASE 	= -D__KERNEL__ -I $(TOPDIR)/include -O
CFLAGS		= $(CFLBASE) -i

CPP		= $(CC) -I$(TOPDIR)/include -E -D__KERNEL__
CC_PROTO	= gcc -I$(MT_DIR)/include -M -D__KERNEL__

LD		= ld86

LINT		= lclint

#########################################################################
# Determine current shell

CONFIG_SHELL := $(shell if [ -x "$$bash" ]; then echo $$bash; \
          else if [ -x /bin/bash ]; then echo /bin/bash; \
          else echo sh; fi ; fi)

#########################################################################
# ROOT_DEV specifies the default root-device when making the image. This
# does not yet work under ELKS. See include/linuxmt/config.h to change
# the root device.

ROOT_DEV	= FLOPPY

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

#########################################################################
# Make "config" the default target if there is no configuration file or
# "dep" the target if there is no top-level dependency information.

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

#########################################################################
# What do we need this time?

ARCHIVES=kernel/kernel.a fs/fs.a lib/lib.a net/net.a

#########################################################################
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

#########################################################################
# Specify how to make the images

Image: $(ARCHIVES) init/main.o
	(cd $(ARCH_DIR); make Image)

nbImage: $(ARCHIVES) init/main.o
	(cd $(ARCH_DIR); make nbImage)

#########################################################################
# Specify how to install the images

nb_install: nbImage
	cp -f $(ARCH_DIR)/boot/nbImage $(TARGET_NB_IMAGE)

nbrd_install: nbImage
	cp -f $(ARCH_DIR)/boot/nbImage $(ARCH_DIR)/boot/nbImage.rd
	cat $(ARCH_DIR)/boot/nbRamdisk >> $(ARCH_DIR)/boot/nbImage.rd
	cp -f $(ARCH_DIR)/boot/nbImage.rd $(TARGET_NB_IMAGE)

boot: Image
	(cd $(ARCH_DIR); make boot)

disk: Image
	(cd $(ARCH_DIR); make disk)

setup: $(ARCH_DIR)/boot/setup  
	(cd $(ARCH_DIR); make setup)

#########################################################################
# library rules (all are built even if they aren't used)

.PHONY: fs/fs.a fs/minix/minixfs.a fs/romfs/romfs.a fs/elksfs/elksfs.a \
        kernel/kernel.a lib/lib.a net/net.a

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
# miscellaneous rules

clean:
	rm -f *~ Boot.map Setup.map System.map tmp_make core 
	rm -f init/*~ init/*.o 
	(cd $(ARCH_DIR); make clean)
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

distclean: clean depclean
	rm -f .config* .menuconfig*
	rm -f scripts/lxdialog/*.o scripts/lxdialog/lxdialog

dist: distdir
	-chmod -R a+r $(DISTDIR)
	tar chozf $(DISTDIR).tar.gz $(DISTDIR)
	-rm -rf $(DISTDIR)

distdir:
	-rm -rf $(DISTDIR)
	mkdir $(DISTDIR)
	-chmod 777 $(DISTDIR)
	cp -pf BUGS CHANGELOG COPYING Makefile $(DISTDIR)
	cp -pf nodeps README RELNOTES TODO $(DISTDIR)
	(mkdir -p $(DISTDIR)/$(ARCH_DIR); cd $(ARCH_DIR); make distdir)
	(mkdir $(DISTDIR)/fs; cd fs;make distdir)
	(mkdir $(DISTDIR)/fs/minix; cd fs/minix;make distdir)
	(mkdir $(DISTDIR)/fs/romfs; cd fs/romfs;make distdir)
	(mkdir $(DISTDIR)/fs/elksfs; cd fs/elksfs;make distdir)
	(mkdir $(DISTDIR)/kernel; cd kernel;make distdir)
	(mkdir $(DISTDIR)/lib; cd lib;make distdir)
	(mkdir $(DISTDIR)/net; cd net;make distdir)
	(mkdir $(DISTDIR)/scripts; cd scripts; make distdir)
	(mkdir -p $(DISTDIR)/include/linuxmt $(DISTDIR)/include/arch)
	cp -pf include/linuxmt/*.h $(DISTDIR)/include/linuxmt
	cp -pf include/arch/*.h $(DISTDIR)/include/arch
	(mkdir -p $(DISTDIR)/init)
	cp -pf init/main.c $(DISTDIR)/init
	(mkdir -p $(DISTDIR)/Documentation)
	cp -a Documentation $(DISTDIR)

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in init/*.c;do echo -n "init/";$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile
	(cd $(ARCH_DIR); make dep)
	(cd fs; make dep)
	(cd fs/minix; make dep)
	(cd fs/romfs; make dep)
	(cd fs/elksfs; make dep)
	(cd kernel; make dep)
	(cd lib; make dep)
	(cd net; make dep)

#########################################################################
# Configuration stuff

menuconfig:  
	make -C scripts/lxdialog all
	$(CONFIG_SHELL) scripts/Menuconfig arch/$(ARCH)/config.in

config:        
	$(CONFIG_SHELL) scripts/Configure arch/$(ARCH)/config.in


include/linuxmt/version.h: ./Makefile
	@echo \#define UTS_RELEASE \"$(VSN)\" > .ver
	@echo \#define ELKS_VERSION_CODE `expr $(PRE) + $(SUBLEVEL) \\* 8 + $(PATCHLEVEL) \\* 256 \\* 8 + $(VERSION) \\* 65536 \\* 8` >> .ver
	sync
	@mv -f .ver $@
	sync

include/linuxmt/compile.h:
	@echo \#define UTS_VERSION \"\#$(VERSION) `date`\" > .ver
	@mv -f .ver $@
	sync

#########################################################################
### Dependencies:
