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

ELKSDIR 	= ../..

#########################################################################
# Include standard commands.

include $(ELKSDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

OBJS  = inode.o

#########################################################################
# Commands.

all:	romfs.a

romfs.a: $(OBJS)
	ar rcs romfs.a $(OBJS)

#########################################################################
# Standard commands.

distdir:
	cp -pf Makefile *.c $(DISTDIR)/fs/romfs

#########################################################################
### Dependencies:
