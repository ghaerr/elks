# Makefile for the Linux/MT-kernel.
#
#########################################################################
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
#########################################################################
# Relative path to base directory.

BASEDIR 	= ../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= Y

CLEANDEP	=

CLEANME 	=

DEPEND  	=

DISTFILES	=

NOINDENT	= user.c

#########################################################################
# Include the standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

OBJS  = init.o malloc.o user.o segment.o

#########################################################################
# Commands.

segment.o:	segment.s

all:	mm.a

mm.a: $(OBJS)
	ar rcs mm.a $(OBJS)
	sync

#########################################################################
# Standard commands.

distdir:
	cp -pf Makefile *.c $(DISTDIR)/arch/i86/mm

#########################################################################
### Dependencies:
