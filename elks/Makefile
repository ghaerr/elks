#########################################################
#							#
#    It is not normally necessary to edit this file.    #
#							#
#########################################################

# Specify the relative path from here to the root of the tree.
# Several other variables are defined based on the definition of this
# variable, so it needs to be accurate.

BASEDIR 	= .

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC  	= Y

CLEANDEP	=

CLEANME 	=

DEPEND  	= include/linuxmt/compiler-generated.h

DISTFILES	= BUGS CHANGELOG COPYING INSTALLATION RELNOTES TODO nodeps

NOINDENT	=

#########################################################################
# Include the standard ruleset.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Targets local to this directory.

all:	do-it-all

# Make "config" the default target if there is no configuration file or
# "depend" the target if there is no top-level dependency information.

#ifeq (.depend,$(wildcard .depend))
#include .depend
do-it-all:      Image tools
#else
#do-it-all:      dep
#endif

#########################################################################
# What do we need this time?

ARCHIVES=fs/fs.a kernel/kernel.a lib/lib.a net/net.a

#########################################################################
# Check what filesystems to include

ifeq ($(CONFIG_MINIX_FS), y)
	ARCHIVES := $(ARCHIVES) fs/minix/minixfs.a
endif

ifeq ($(CONFIG_ROMFS_FS), y)
	ARCHIVES := $(ARCHIVES) fs/romfs/romfs.a
endif

ifeq ($(CONFIG_FS_FAT), y)
	ARCHIVES := $(ARCHIVES) fs/msdos/msdos.a
endif

#########################################################################
# Define commands.

Image: $(TOPDIR)/include/autoconf.h tools
	$(MAKE) $(ARCHIVES)
	$(MAKE) init/main.o
	$(MAKE) -C $(ARCH_DIR) Image

nbImage: $(TOPDIR)/include/autoconf.h $(ARCHIVES) init/main.o tools
	${MAKE} -C $(ARCH_DIR) nbImage

nb_install: nbImage
	cp -f $(ARCH_DIR)/boot/nbImage $(TARGET_NB_IMAGE)

nbrd_install: nbImage
	cp -f $(ARCH_DIR)/boot/nbImage $(ARCH_DIR)/boot/nbImage.rd
	cat $(ARCH_DIR)/boot/nbRamdisk >> $(ARCH_DIR)/boot/nbImage.rd
	cp -f $(ARCH_DIR)/boot/nbImage.rd $(TARGET_NB_IMAGE)

boot: Image
	${MAKE} -C $(ARCH_DIR) boot

disk: Image
	${MAKE} -C $(ARCH_DIR) disk

setup: $(ARCH_DIR)/boot/setup
	${MAKE} -C $(ARCH_DIR) setup

#########################################################################
# library rules (all these are built even if they aren't used).

.PHONY: fs/fs.a fs/msdos/msdos.a fs/minix/minixfs.a fs/romfs/romfs.a \
        kernel/kernel.a lib/lib.a net/net.a tools

fs/fs.a:
	${MAKE} -C fs fs.a

fs/msdos/msdos.a:
	${MAKE} -C fs/msdos msdos.a

fs/minix/minixfs.a:
	${MAKE} -C fs/minix minixfs.a

fs/romfs/romfs.a: tools
	${MAKE} -C fs/romfs romfs.a

kernel/kernel.a:	include/linuxmt/compiler-generated.h
	@sed 's/^/ Q> /' < include/linuxmt/compiler-generated.h
	${MAKE} -C kernel kernel.a

lib/lib.a:
	${MAKE} -C lib lib.a

net/net.a:
	${MAKE} -C net net.a

tools:
	${MAKE} -C tools all
	-rm -f include/linuxmt/compiler-generated.h
	-rm -f kernel/version.o

#########################################################################
# Compiler-generated definitions not given as command arguments.

include/linuxmt/compiler-generated.h:
	printf > include/linuxmt/compiler-generated.h		\
		'#define UTS_RELEASE %s\n' "\"$(DIST)\""
	printf >> include/linuxmt/compiler-generated.h		\
		'#define UTS_VERSION %s\n' "\"$(shell git log --abbrev-commit | head -1 | sed 's/commit/commit/') $(shell date '+%d %b %Y %T %z')\""

#########################################################################
# lint rule

xlint:	$(TOPDIR)/include/autoconf.h
	@echo
	$(LINT) -I$(TOPDIR)/elks/include -c init/main.c
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
	@${MAKE} -C . nodep clean distclean
	@echo
	-rm -rf $(DISTDIR)
	@${MAKE} mkdist

dist.old:
	mkdir -m 755 -p $(DISTDIR)
	cp -pf BUGS BUGS.html CHANGELOG COPYING nodeps $(DISTDIR)
	cp -pf config.in Makefile Makefile-rules README RELNOTES $(DISTDIR)
	cp -pf INSTALLATION INSTALLATION.html TODO $(DISTDIR)
	(cd $(DISTDIR); mkdir fs include init kernel lib net)
	(cd $(DISTDIR); mkdir -p $(ARCH_DIR) scripts)
	(cd $(DISTDIR)/fs; mkdir minix romfs)
	(cd $(DISTDIR)/include; mkdir arch linuxmt)
	${MAKE} -C $(ARCH_DIR) distdir
	${MAKE} -C fs distdir
	${MAKE} -C fs/minix distdir
	${MAKE} -C fs/romfs distdir
	${MAKE} -C kernel distdir
	${MAKE} -C lib distdir
	${MAKE} -C net distdir
	${MAKE} -C scripts distdir
	cp -pf include/linuxmt/*.h $(DISTDIR)/include/linuxmt
	cp -pf include/arch/*.h $(DISTDIR)/include/arch
	cp -pf init/main.c $(DISTDIR)/init
	@echo
	@echo Directory $(DISTDIR) now contains a clean distribution tree.
	@echo
	@printf '\n  %076u\n\n' 0 | tr 0 =

distclean: clean
	rm -f arch/i86/Image

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

kclean: dokclean

.NOTPARALLEL: clean
clean:	nodep doclean

dep:	$(TOPDIR)/include/autoconf.h
	@$(MAKE) dodep
	@echo
	@echo ' * Dependencies configured. Next, you need to run one of the'
	@echo ' * following commands:'
	@echo ' *'
	@echo ' *    `make Image`     to create a kernel image ready to boot'
	@echo ' *                     from floppy diskette, or'
	@echo ' *'
	@echo ' *    `make nbImage`   to create a kernel image ready to boot'
	@echo ' *                     across a network connection.'
	@echo

dup:
	rm -fr ../elks-test
	mkdir ../elks-test
	tar c * | ( cd ../elks-test ; tar xv )
	@echo
	@echo ' * Created backup tree for testing.'
	@echo

#########################################################################
#
# The following command performs a test of all the major commands used in
# this system. The steps taken are as follows:
#
#	make dist	Create a distribution tree from the current set
#			of files. This creates the PRE distribution.
#
#	make clean	Remove all of the compilation junk, so we know
#			we're starting from scratch.
#
#	make dep	Create a full set of dependencies.
#
#	make Image	Compile ELKS and create the standard image file.
#
#	make nbImage	Compile ELKS and create the NetBoot Image file.
#
#	make nodep	Remove all of the dependency information.
#
#	make dist	Create a distribution tree from the current set
#			of files. This creates the POST distribution.
#
#	diff ...	Compare the PRE and POST distributions. In theory
#			these will be identical, so any output from this
#			stage indicates a problem somewhere.
#
# Note that the output from each of the above steps is separated by a row
# of = signs that stretches across the screen.
#
#########################################################################

test:
	${MAKE} dist
	mv $(DISTDIR){,.Pre}
	@printf '\n  %076u\n\n' 0 | tr 0 =
	${MAKE} clean
	@printf '\n  %076u\n\n' 0 | tr 0 =
	${MAKE} dep
	@printf '\n  %076u\n\n' 0 | tr 0 =
	${MAKE} Image
	@printf '\n  %076u\n\n' 0 | tr 0 =
	${MAKE} nbImage
	@printf '\n  %076u\n\n' 0 | tr 0 =
	${MAKE} dist
	mv $(DISTDIR){,.Post}
	@printf '\n  %076u\n\n' 0 | tr 0 =
	diff -ur $(DISTDIR).{Pre,Post} || true

#########################################################################
### Dependencies:
