# Makefile for bcc library - 8086 assembler routines
#
#########################################################################
# Relative path to base directory.

BASEDIR 	= ../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 	= Y

CLEANDEP	= 

CLEANME 	= 

DEPEND		= 

DISTFILES	= 

#########################################################################
# Include the standard rules.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

OBJS		= $(IOBJS) $(JOBJS) $(LLOBJS) $(LXOBJS)

# compiler support for integer arithmetic

IOBJS		=idiv.o idivu.o imod.o imodu.o imul.o isl.o isr.o isru.o

# miscellaneous

JOBJS		=inport.o inportb.o outport.o outportb.o \
		 peekb.o peekw.o peekd.o pokeb.o pokew.o poked.o bitops.o \
		 memmove.o string.o fmemset.o border.o
#		 ntohl.o ntohs.o

# compiler support for long arithmetic on little-endian (normal) longs

LLOBJS		=laddl.o landl.o lcmpl.o lcoml.o ldecl.o ldivl.o ldivul.o \
		 leorl.o lincl.o lmodl.o lmodul.o lmull.o lnegl.o lorl.o \
		 lsll.o lsrl.o lsrul.o lsubl.o ltstl.o setupw.o setupb.o

# compiler support for long arithmetic on all longs

LXOBJS		=ldivmod.o

#########################################################################
# Commands.

all:	lib86.a

lib86.a: $(OBJS)
	ar rcs lib86.a $(OBJS)
	sync

setupb.s: setupb.S

setupw.s: setupw.S

#########################################################################
# Standard commands.

distdir:
	cp -pf Makefile *.c *.S *.s $(DISTDIR)/arch/i86/lib

#########################################################################
### Dependencies:
