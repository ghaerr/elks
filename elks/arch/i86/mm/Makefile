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

ELKSDIR 	= ../../..

#########################################################################
# Include standard commands.

include $(ELKSDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

OBJS  = init.o segment.o malloc.o user.o

#########################################################################
# Commands.

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
