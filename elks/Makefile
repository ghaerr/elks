# Note that the standard Makefile rules and defines have been moved to
# the file below.

ELKSDIR		= .

include $(ELKSDIR)/Makefile-rules

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
# Check what filesystems to include, in ASCII order.

ifeq ($(CONFIG_ELKSFS_FS), y)
	ARCHIVES := $(ARCHIVES) fs/elksfs/elksfs.a
endif

ifeq ($(CONFIG_MINIX_FS), y)
	ARCHIVES := $(ARCHIVES) fs/minix/minixfs.a
endif

ifeq ($(CONFIG_ROMFS_FS), y)
	ARCHIVES := $(ARCHIVES) fs/romfs/romfs.a
endif

#########################################################################
# Specify how to make the images, in ASCII order.

Image: $(ARCHIVES) init/main.o
	make -C $(ARCH_DIR) Image

nbImage: $(ARCHIVES) init/main.o
	make -C $(ARCH_DIR) nbImage

#########################################################################
# Specify how to install the images, in ASCII order.

boot: Image
	make -c $(ARCH_DIR) boot

disk: Image
	make -c $(ARCH_DIR) disk

nb_install: nbImage
	cp -f $(ARCH_DIR)/boot/nbImage $(TARGET_NB_IMAGE)

nbrd_install: nbImage
	cp -f $(ARCH_DIR)/boot/nbImage $(ARCH_DIR)/boot/nbImage.rd
	cat $(ARCH_DIR)/boot/nbRamdisk >> $(ARCH_DIR)/boot/nbImage.rd
	cp -f $(ARCH_DIR)/boot/nbImage.rd $(TARGET_NB_IMAGE)

setup: $(ARCH_DIR)/boot/setup  
	make -c $(ARCH_DIR) setup

#########################################################################
# library rules (all are built even if they aren't used)

.PHONY: fs/fs.a fs/minix/minixfs.a fs/romfs/romfs.a fs/elksfs/elksfs.a \
        kernel/kernel.a lib/lib.a net/net.a

fs/elksfs/elksfs.a:
	make -C elksfs all

fs/fs.a:
	make -C fs all

fs/minix/minixfs.a:
	make -C minix all

fs/romfs/romfs.a:
	make -C romfs all

kernel/kernel.a: include/linuxmt/version.h include/linuxmt/compile.h
	make -C kernel all

lib/lib.a:
	make -C lib all

net/net.a:
	make -C net all

#########################################################################
# lint rules

lint:
	$(LINT) -I$(MT_DIR)/include -c init/main.c

#########################################################################
# miscellaneous rules

clean:
	rm -f *~ Boot.map Setup.map System.map tmp_make core 
	rm -f init/*~ init/*.o 
	make -C $(ARCH_DIR) clean
	make -C fs clean
	make -C fs/elksfs clean
	make -C fs/minix clean
	make -C fs/romfs clean
	make -C kernel clean
	make -C lib clean
	make -C net clean
	make -C scripts clean

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
	(mkdir -p $(DISTDIR)/$(ARCH_DIR); make -C $(ARCH_DIR) distdir)
	(mkdir $(DISTDIR)/fs; make -C fs distdir)
	(mkdir $(DISTDIR)/fs/minix; make -C fs/minix distdir)
	(mkdir $(DISTDIR)/fs/romfs; make -C fs/romfs distdir)
	(mkdir $(DISTDIR)/fs/elksfs; make -C fs/elksfs distdir)
	(mkdir $(DISTDIR)/kernel; make -C kernel distdir)
	(mkdir $(DISTDIR)/lib; make -C lib distdir)
	(mkdir $(DISTDIR)/net; make -C net distdir)
	(mkdir $(DISTDIR)/scripts; make -C scripts distdir)
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
	make -C $(ARCH_DIR) dep
	make -C fs dep
	make -C fs/elksfs dep
	make -C fs/minix dep
	make -C fs/romfs dep
	make -C kernel dep
	make -C lib dep
	make -C net dep

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
