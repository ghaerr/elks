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

OBJS  = init.o ll_rw_blk.o genhd.o hdinfo.o
OBJS += bioshd.o bios.o             # CONFIG_BLK_DEV_BFD / CONFIG_BLK_DEV_BHD
OBJS += ata.o ata-cf.o              # CONFIG_BLK_DEV_ATA_CF
OBJS += rd.o                        # CONFIG_BLK_DEV_RAM
OBJS += idequery.o                  # CONFIG_IDE_PROBE
OBJS += romflash.o                  # CONFIG_ROMFS_FS

ifdef CONFIG_ASYNCIO
OBJS += directfd.o                  # CONFIG_BLK_DEV_FD
endif

ifdef CONFIG_ARCH_IBMPC
OBJS += mfmhd.o                     # CONFIG_BLK_DEV_MFMHD
endif

ifdef CONFIG_FS_XMS
  ifdef CONFIG_FS_XMS_RAMDISK
    OBJS += ssd.o ssd-xms.o
  endif
endif

ifdef CONFIG_BLK_DEV_SSD_TEST
OBJS += ssd.o ssd-test.o
endif

ifdef CONFIG_BLK_DEV_SSD_SD
  ifdef CONFIG_ARCH_NECV25
    ifdef CONFIG_HW_SPI
      OBJS += ssd.o ssd-sd.o spi-hw-necv25.o
    else
      OBJS += ssd.o ssd-sd.o spi-necv25.o
    endif
  endif
  ifdef CONFIG_ARCH_8018X
    OBJS += ssd.o ssd-sd.o spi-8018x.o
  endif
endif

#########################################################################
# Commands.

all:	blk_drv.a

blk_drv.a: $(OBJS)
	$(AR) rcs blk_drv.a $(OBJS)

#########################################################################
# Standard commands.

#########################################################################
### Dependencies:
