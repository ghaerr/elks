# Convert ELF output file from gold linker to ELKS a.out
# Requires libelf to be installed
#
#########################################################################
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
#########################################################################
# Relative path to base directory.

BASEDIR 	= ../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= N

CLEANDEP	=

CLEANME 	= ../bin/elf2elks

DEPEND  	=

DISTFILES	=

NOINDENT	=

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

ELFTOOLCHAIN = lib/elftoolchain-0.7.1
INCLUDES	+= -I$(TOPDIR)/elks/tools/elf2elks/$(ELFTOOLCHAIN)/libelf \
		   -I$(TOPDIR)/elks/tools/elf2elks/$(ELFTOOLCHAIN)/common

#########################################################################
# Objects to be compiled.

LIBELF		= $(ELFTOOLCHAIN)/libelf/libelf.a
OBJS		= elf2elks.o $(LIBELF)

#########################################################################
# Commands.

all:	../bin/elf2elks

../bin/elf2elks: $(OBJS)
	$(CC) -o ../bin/elf2elks $(CFLAGS) $(OBJS)

elf2elks.o: elf2elks.c $(LIBELF)

# libelf has its own makefile for building, but the makefile requires BSD
# make.  To avoid roping in BSD make as yet another dependency, we directly
# run the commands to generate and compile the libelf sources here.  This
# still requires m4 as a dependency though.
$(LIBELF): $(ELFTOOLCHAIN).tar.bz2 lib/elftoolchain.patch lib/elftoolchain2.patch
	rm -rf $(ELFTOOLCHAIN)
	cd lib && tar -xvjf elftoolchain-0.7.1.tar.bz2
	cd $(ELFTOOLCHAIN) && patch -p1 <'$(abspath lib/elftoolchain.patch)' && patch -p1 <'$(abspath lib/elftoolchain2.patch)'
ifeq "Linux" "$(shell uname -s)"
	set -e; \
	cd $(ELFTOOLCHAIN)/common; \
	./native-elf-format > native-elf-format.h
endif
	set -e; \
	cd $(ELFTOOLCHAIN)/libelf; \
	m4 -DSRCDIR=. libelf_convert.m4 > libelf_convert.c; \
	m4 -DSRCDIR=. libelf_fsize.m4 > libelf_fsize.c; \
	m4 -DSRCDIR=. libelf_msize.m4 > libelf_msize.c; \
	for src in *.c; \
		do $(CC) $(CFLAGS) -c -I. -I../common "$$src"; done; \
	rm -f libelf.a; \
	$(AR) $(ARFLAGS) libelf.a *.o

doclean: elftoolchain-doclean

elftoolchain-doclean:
	rm -rf $(ELFTOOLCHAIN)

#########################################################################
### Dependencies:
