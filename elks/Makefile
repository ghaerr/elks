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

distclean: clean
	rm -f arch/i86/Image

#########################################################################
# Create distribution archives.


#########################################################################
# Configuration stuff

kclean: dokclean

.NOTPARALLEL: clean
clean:	nodep doclean

dep:	$(TOPDIR)/include/autoconf.h
	@$(MAKE) dodep
	@echo
	@echo ' * Dependencies configured. Next, you need to run the'
	@echo ' * following command:'
	@echo ' *'
	@echo ' *    `make Image`     to create a kernel image ready to boot'
	@echo ' *                     from floppy diskette, or'
	@echo

#########################################################################
#
#	make dist	Create a distribution tree from the current set
#			of files.
#
#	make clean	Remove all of the compilation junk, so we know
#			we're starting from scratch.
#
#	make dep	Create a full set of dependencies.
#
#	make Image	Compile ELKS and create the standard image file.
#
#	make nodep	Remove all of the dependency information.
#
#	make dist	Create a distribution tree from the current set
#			of files. This creates the POST distribution.
#
#########################################################################


#########################################################################
### Dependencies:
