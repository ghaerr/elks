# Build the Psion specific stuff.
#
#########################################################################
# Relative path to base directory.

BASEDIR 	= ../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 	= Y

CLEANDEP	= 

CLEANME 	= 

DEPEND		= 

DISTFILES	= defconfig

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# SIBO specific options.

ifeq ($(CONFIG_ARCH_SIBO), y)

SIBOFLAGS 	= -D 0xc00 -H 0x480

else

SIBOFLAGS	=

endif

LDFLAGS 	= -0 -i $(SIBOFLAGS)

#########################################################################
# Objects to be compiled.

AARCHIVES = kernel/akernel.a lib/lib86.a mm/mm.a

ARCHIVES := $(ARCHIVES) $(ARCH_DIR)/kernel/akernel.a \
	$(ARCH_DIR)/lib/lib86.a \
	$(ARCH_DIR)/mm/mm.a

ADRIVERS = drivers/char/chr_drv.a drivers/block/blk_drv.a

DRIVERS := $(DRIVERS) $(ARCH_DIR)/drivers/char/chr_drv.a \
	$(ARCH_DIR)/drivers/block/blk_drv.a

#########################################################################
# Things to make.

.PHONY: 		boot/system tools

boot/bootsect:		boot/bootsect.o
	$(LD) -0 $(ARCH_LD) -s -o boot/bootsect -M boot/bootsect.o > Boot.map

boot/bootsect.o:	boot/bootsect.s

boot/crt0.o:		boot/crt0.s

boot/crt1.o:		boot/crt1.c

boot/netbootsect:	boot/netbootsect.o
	$(LD) $(LDFLAGS) $(ARCH_LD) -s -o boot/netbootsect -M boot/netbootsect.o > Netboot.map

boot/netbootsect.o:	boot/netbootsect.s

boot/setup:		boot/setup.o
	$(LD) -0 $(ARCH_LD) -s -o boot/setup -M boot/setup.o > Setup.map


boot/setup.o:		boot/setup.s

tools:
	make -C tools all

#########################################################################
# Image selection.

ifeq ($(CONFIG_ARCH_SIBO), y)

boot/system: $(AARCHIVES) $(ADRIVERS) sibo/crt1.o sibo/crt0.o
	(cd $(BASEDIR) ; $(LD) $(LDFLAGS) $(ARCH_LD) \
		$(ARCH_DIR)/sibo/crt0.o $(ARCH_DIR)/sibo/crt1.o \
		init/main.o $(ARCHIVES) $(DRIVERS) \
		-t -M -o $(ARCH_DIR)/boot/system > System.tmp ; \
		sort -k4 System.tmp > System.map ; rm -f System.tmp ) 

#SIBO image build
Image: boot/system tools
	(cd boot; ../tools/imgconv system system.app -S192 -H72 -V)

#End SIBO image section

else

boot/system:	$(AARCHIVES) $(ADRIVERS) boot/crt1.o boot/crt0.o
	(cd $(BASEDIR) ; $(LD) $(LDFLAGS) $(ARCH_LD) \
		$(ARCH_DIR)/boot/crt0.o $(ARCH_DIR)/boot/crt1.o \
		init/main.o $(ARCHIVES) $(DRIVERS) \
		-t -M -o $(ARCH_DIR)/boot/system > System.tmp ; \
		sort -k4 System.tmp > System.map ; rm -f System.tmp ) 

#PC image build

ifneq ($(CONFIG_ROMCODE), y)

ifeq ($(CONFIG_286PMODE), y)

Image:		tools/build boot/bootsect boot/setup 286pmode/pmode286 boot/system tools
	tools/build boot/bootsect boot/setup 286pmode/pmode286 boot/system $(ROOT_DEV) > Image

else

Image:		boot/bootsect boot/setup boot/system tools
	tools/build boot/bootsect boot/setup boot/system $(ROOT_DEV) > boot/Image

nbImage:	Image boot/netbootsect
	tools/mknbi-elks boot/netbootsect boot/Image boot/nbImage

endif

else

ifeq ($(CONFIG_ROM_BOOTABLE_BY_RESET), y)
    RVECT = -r $(CONFIG_ROM_RESET_ADDRESS)   
endif

Image:	tools boot/setup boot/system  
	tools/mkurlader $(RVECT) -c $(CONFIG_ROM_SETUP_CODE) $(CONFIG_ROM_CHECKSUM_SIZE) Image $(CONFIG_ROM_BASE) -a boot/setup $(CONFIG_ROM_SETUP_CODE) -s boot/system $(CONFIG_ROM_KERNEL_CODE) $(CONFIG_ROM_BIOS_MODULE) $(CONFIG_ROM_BIOS_MODULE_ADDR)
	sync
	$(CONFIG_ROM_SIMULATOR_PROGRAM)

endif

#########################################################################
# Other commands.

boot:	Image
	dd if=boot/Image of=/dev/fd0 bs=8192

disk:	boot

setup:	$(ARCH_DIR)/tools $(ARCH_DIR)/boot/setup  
	tools/mkurlader $(RVECT) -c $(CONFIG_ROM_SETUP_CODE) $(ROM_MAX_SETUP_SIZE) Image $(CONFIG_ROM_BASE) -a boot/setup $(CONFIG_ROM_SETUP_CODE) -s boot/system $(CONFIG_ROM_KERNEL_CODE) $(CONFIG_ROM_BIOS_MODULE) $(CONFIG_ROM_BIOS_MODULE_ADDR)
	sync
	$(CONFIG_ROM_SIMULATOR_PROGRAM)
endif

#########################################################################
# library rules

# Build archives and add them to the list of things to link into kernel image

.PHONY: kernel/akernel.a lib/lib86.a mm/mm.a drivers/char/chr_drv.a \
	drivers/block/blk_drv.a 286pmode/pmode286

kernel/akernel.a:
	make -C kernel akernel.a

lib/lib86.a:
	make -C lib lib86.a

mm/mm.a:
	make -C mm mm.a

drivers/char/chr_drv.a:
	make -C drivers/char chr_drv.a

drivers/block/blk_drv.a:
	make -C drivers/block blk_drv.a

286pmode/pmode286:
	make -C 286pmode pmode286

#########################################################################
# commands.

distdir:
	mkdir -p $(DISTDIR)/arch/i86
	(cd $(DISTDIR)/arch/i86 ; \
		mkdir -p 286pmode boot drivers kernel lib mm tools)
	(cd $(DISTDIR)/arch/i86/drivers ; \
		mkdir -p block char)
	cp -pf config.in defconfig Makefile $(DISTDIR)/arch/i86
	cp -pf boot/*.S boot/*.c boot/tux.pic $(DISTDIR)/arch/i86/boot
	cp -pf tools/Makefile tools/*.h $(DISTDIR)/arch/i86/tools
	cp -pf tools/mknbi-elks tools/*.c $(DISTDIR)/arch/i86/tools
	cp -pf tools/*.s tools/*.1 $(DISTDIR)/arch/i86/tools
	make -C 286pmode distdir
	make -C drivers/block distdir
	make -C drivers/char distdir
	make -C kernel distdir
	make -C lib distdir
	make -C mm distdir

#########################################################################
### Dependencies:
