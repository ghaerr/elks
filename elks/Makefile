#########################################################
#							#
#    It is not normally necessary to edit this file.    #
#							#
#########################################################

# Specify the relative path from here to the root of the ELKS tree.
# Several other variables are defined based on the definition of this
# variable, so it needs to be accurate.

ELKSDIR		= .

#########################################################################
# Include the standard ruleset.

include $(ELKSDIR)/Makefile-rules

#########################################################################
# Targets local to this directory.

all:	do-it-all

# Make "config" the default target if there is no configuration file or
# "depend" the target if there is no top-level dependency information.

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
# Define commands.

Image: $(ARCHIVES) init/main.o
	make -C $(ARCH_DIR) Image

nbImage: $(ARCHIVES) init/main.o
	make -C $(ARCH_DIR) nbImage

nb_install: nbImage
	cp -f $(ARCH_DIR)/boot/nbImage $(TARGET_NB_IMAGE)

nbrd_install: nbImage
	cp -f $(ARCH_DIR)/boot/nbImage $(ARCH_DIR)/boot/nbImage.rd
	cat $(ARCH_DIR)/boot/nbRamdisk >> $(ARCH_DIR)/boot/nbImage.rd
	cp -f $(ARCH_DIR)/boot/nbImage.rd $(TARGET_NB_IMAGE)

boot: Image
	make -C $(ARCH_DIR) boot

disk: Image
	make -C $(ARCH_DIR) disk

setup: $(ARCH_DIR)/boot/setup
	make -C $(ARCH_DIR) setup

#########################################################################
# library rules (all these are built even if they aren't used)

.PHONY: fs/fs.a fs/minix/minixfs.a fs/romfs/romfs.a fs/elksfs/elksfs.a \
        kernel/kernel.a lib/lib.a net/net.a

fs/fs.a:
	make -C fs fs.a

fs/elksfs/elksfs.a:
	make -C fs/elksfs elksfs.a

fs/minix/minixfs.a:
	make -C fs/minix minixfs.a

fs/romfs/romfs.a:
	make -C fs/romfs romfs.a

kernel/kernel.a:	include/linuxmt/compiler-generated.h
	make -C kernel kernel.a

lib/lib.a:
	make -C lib lib.a

net/net.a:
	make -C net net.a

#########################################################################
# Compiler-generated definitions not given as command arguments.

include/linuxmt/compiler-generated.h:
	printf > include/linuxmt/compiler-generated.h	\
		'#define %s "%s"\n'			\
		UTS_VERSION "#$(DIST) $(shell date)"

#########################################################################
# lint rule

lint:
	$(LINT) -I$(TOPDIR)/include -c init/main.c
	@echo
	@echo Checking with lint is now complete.
	@echo

#########################################################################
# Specification files for archives.

elks.spec: Makefile
	@scripts/elksspec $(DISTDIR) $(DIST) $(shell date +%Y.%m.%d)

#########################################################################
# Standard commands.

dist:
	-rm -rf $(DISTDIR)
	mkdir $(DISTDIR)
	-chmod 777 $(DISTDIR)
	cp -pf BUGS CHANGELOG COPYING Makefile Makefile-rules $(DISTDIR)
	cp -pf nodeps README RELNOTES TODO $(DISTDIR)
	(cd $(DISTDIR); mkdir Documentation fs include init kernel lib net)
	(cd $(DISTDIR); mkdir -p $(ARCH_DIR) scripts)
	(cd $(DISTDIR)/fs; mkdir elksfs minix romfs)
	(cd $(DISTDIR)/include; mkdir arch linuxmt)
	make -C $(ARCH_DIR) distdir
	make -C fs distdir
	make -C fs/elksfs distdir
	make -C fs/minix distdir
	make -C fs/romfs distdir
	make -C kernel distdir
	make -C lib distdir
	make -C net distdir
	make -C scripts distdir
	cp -pf include/linuxmt/*.h $(DISTDIR)/include/linuxmt
	cp -pf include/arch/*.h $(DISTDIR)/include/arch
	cp -pf init/main.c $(DISTDIR)/init
	cp -apf Documentation $(DISTDIR)
	@echo
	@echo Directory $(DISTDIR) contains a clean ELKS distribution tree.
	@echo

distclean: clean nodep
	rm -f .config* .menuconfig*
	rm -f scripts/lxdialog/*.o scripts/lxdialog/lxdialog
	@echo
	@echo This ELKS source tree has been cleaned ready for distribution.
	@echo

#########################################################################
# Create distribution archives.

deb:	tar
	@echo
	@echo I do not yet know how to create *.deb archives, sorry.
	@echo

rpm:	tar rpm.spec
	@echo
	@echo I do not yet know how to create *.rpm archives, sorry.
	@echo I have, however, created the spec file required to do so,
	@echo and the tar archive to be placed inside it.
	@echo

tar:	dist
	-chmod -R a+r $(DISTDIR)
	tar chozf $(DISTDIR).tar.gz $(DISTDIR)
	-rm -rf $(DISTDIR)

#########################################################################
# Configuration stuff

config:
	@echo
	@$(CONFIG_SHELL) scripts/Configure arch/$(ARCH)/config.in
	@echo
	@echo Configuration complete.
	@echo

defconfig:
	@echo
	@yes '' | make config
	@echo
	@echo Configuration complete.
	@echo

me:
	rm -fr ../elks-test
	mkdir ../elks-test
	tar c * | ( cd ../elks-test ; tar xv )
	@echo
	@echo Created backup tree for testing.
	@echo

menuconfig:
	make -C scripts/lxdialog all
	$(CONFIG_SHELL) scripts/Menuconfig arch/$(ARCH)/config.in
	@echo
	@echo Configuration complete.
	@echo

set:
	@echo
	@set
	@echo

test:
	make defconfig
	@printf '\n%079u\n\n' 0 | tr 0 =
	make dep
	@printf '\n%079u\n\n' 0 | tr 0 =
	make clean
	@printf '\n%079u\n\n' 0 | tr 0 =
	make Image
	@printf '\n%079u\n\n' 0 | tr 0 =
	make nbImage
	@printf '\n%079u\n\n' 0 | tr 0 =
	make dist

#########################################################################
### Dependencies:
