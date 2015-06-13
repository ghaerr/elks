# Makefile for the ELKS blocks devices.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
#########################################################################
# Relative path to base directory.

BASEDIR 	= ../../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= Y

CLEANDEP	= 

CLEANME 	= 

DEPEND  	= 

DISTFILES	= 

NOINDENT	= directhd.c

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

ifeq ($(CONFIG_ARCH_SIBO), y)
OBJS  = init.o genhd.o ll_rw_blk.o rd.o sibo_ssd.o ssd_asm.o
else
OBJS  = init.o genhd.o ll_rw_blk.o rd.o doshd.o floppy.o directhd.o

endif

#########################################################################
# Commands.

all:	blk_drv.a

blk_drv.a: $(OBJS)
	ar rcs blk_drv.a $(OBJS)

ssd_asm.o: ssd_asm.s

ssd_asm.s: ssd_asm.S

#########################################################################
# Standard commands.

distdir:
	cp -pf Config.in Makefile $(DISTDIR)/arch/i86/drivers/block
	cp -pf *.S *.h *.c $(DISTDIR)/arch/i86/drivers/block

#########################################################################
### Dependencies:
