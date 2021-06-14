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

OBJS  = init.o genhd.o ll_rw_blk.o

# BIOS floppy and hard drive support
ifeq ($(CONFIG_BLK_DEV_BIOS), y)
OBJS += bioshd.o idequery.o
endif

ifeq ($(CONFIG_BLK_DEV_RAM), y)
OBJS += rd.o
endif

ifeq ($(CONFIG_ROMFS_FS), y)
OBJS += romflash.o
endif

ifeq ($(CONFIG_BLK_DEV_SSD), y)
OBJS += ssd.o ssd_test.o
endif

# experimental (and not working) direct hd support
ifeq ($(CONFIG_BLK_DEV_HD), y)
OBJS += directhd.o
endif
# experimental (and not compiling) direct fd support
ifeq ($(CONFIG_BLK_DEV_FD), y)
OBJS += directfd.o
endif

#########################################################################
# Commands.

all:	blk_drv.a

blk_drv.a: $(OBJS)
	$(AR) rcs blk_drv.a $(OBJS)

#########################################################################
# Standard commands.

distdir:
	cp -pf config.in Makefile $(DISTDIR)/arch/i86/drivers/block
	cp -pf *.S *.h *.c $(DISTDIR)/arch/i86/drivers/block

#########################################################################
### Dependencies:
