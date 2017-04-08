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

BASEDIR 	= ../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= Y

CLEANDEP	= 

CLEANME 	= 

DEPEND  	= 

DISTFILES	= 

NOINDENT	= 

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

OBJS  = namei.o inode.o file.o dir.o misc.o fat.o

#CFLAGS += -DLINUX_FS_2_0_34 
#CFLAGS += -DCONFIG_UMSDOS_FS
#CFLAGS += -DFAT_BITS_32
ifeq ($(findstring -DLINUX_FS_2_0_34, $(CFLAGS)),-DLINUX_FS_2_0_34)
namei.o: ../vfat/namei.c 
	$(CC) $(CFLAGS) -c -o $@ $<
endif

#########################################################################
# Commands.

all:	msdos.a

msdos.a: $(OBJS)
	ar rcs msdos.a $(OBJS)

#########################################################################
# Standard commands.

distdir:
	cp -pf Makefile *.c $(DISTDIR)/fs/msdos

#########################################################################
### Dependencies:
