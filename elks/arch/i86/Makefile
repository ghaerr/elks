# Build the Psion specific stuff.
#
#########################################################################
# Relative path to base directory.

BASEDIR 	= ../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC  	= Y

CLEANDEP	= 

CLEANME 	= $(BASEDIR)/include/arch/asm-offsets.h

DEPEND  	= 

DISTFILES	= defconfig

NOINDENT	= 

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

#########################################################################
# Objects to be compiled.

AARCHIVES = kernel/akernel.a lib/lib86.a mm/mm.a

ARCHIVES := $(ARCHIVES) $(ARCH_DIR)/kernel/akernel.a \
	$(ARCH_DIR)/lib/lib86.a \
	$(ARCH_DIR)/mm/mm.a

ADRIVERS = drivers/char/chr_drv.a drivers/block/blk_drv.a drivers/net/net_drv.a

DRIVERS := $(DRIVERS) \
	$(ARCH_DIR)/drivers/char/chr_drv.a \
	$(ARCH_DIR)/drivers/block/blk_drv.a \
	$(ARCH_DIR)/drivers/net/net_drv.a

XINCLUDE = $(BASEDIR)/include/arch/asm-offsets.h

#########################################################################
# Things to make.

.PHONY: toolkit

boot/bootsect.o: boot/bootsect.S
boot/bootsect.o: boot/bootsect.s
boot/bootsect:   boot/bootsect.o
	$(LD) $(LDFLAGS) $(SIBOFLAGS) -M -T $(TOPDIR)/elks/elks-tiny.ld -o boot/bootsect boot/bootsect.o > boot/bootsect.map

boot/netbootsect:	boot/netbootsect.o
	$(LD) $(LDFLAGS) $(SIBOFLAGS) -M $(ARCH_LD) -s -o boot/netbootsect boot/netbootsect.o > boot/netbootsect.map

boot/netbootsect.o:	boot/netbootsect.s

boot/setup.s: boot/setup.S
boot/setup.o: boot/setup.s
boot/setup:   boot/setup.o
	$(LD) $(LDFLAGS) $(SIBOFLAGS) -M -T $(TOPDIR)/elks/elks-tiny.ld -o boot/setup boot/setup.o > boot/setup.map

boot/crt0.s:		boot/crt0.S
boot/crt0.o:		boot/crt0.s

boot/crt1.o:		boot/crt1.c

toolkit:
	${MAKE} -C tools all

$(BASEDIR)/include/arch/asm-offsets.h: kernel/asm-offsets.c
	echo '#ifndef ASM_OFFSETS_H' > $(BASEDIR)/include/arch/asm-offsets.h
	echo '#define ASM_OFFSETS_H' >> $(BASEDIR)/include/arch/asm-offsets.h
	$(CC) $(CFLAGS) -S -o asm-offsets.s kernel/asm-offsets.c
	sed -e '/^\tm/ !d' \
	-e "s/\(.*\t\\$$\)/\#define /" \
	-e "s/\([0-9][0-9]*\),\t\([a-zA-Z][_0-9a-zA-Z]*\)/\2 \1/" \
	< asm-offsets.s >> $(BASEDIR)/include/arch/asm-offsets.h
	echo '#endif' >> $(BASEDIR)/include/arch/asm-offsets.h
	rm asm-offsets.s

#########################################################################
# Image selection.

ifeq ($(CONFIG_ARCH_SIBO), y)

# Begin SIBO image build

boot/system: $(XINCLUDE) $(AARCHIVES) $(ADRIVERS) sibo/crt1.o sibo/crt0.o
	(cd $(BASEDIR) ; $(LD) $(LDFLAGS) $(SIBOFLAGS) -M $(ARCH_LD) \
		$(ARCH_DIR)/sibo/crt0.o $(ARCH_DIR)/sibo/crt1.o \
		init/main.o $(ARCHIVES) $(DRIVERS) \
		-o $(ARCH_DIR)/boot/system > $(ARCH_DIR)/boot/system.tmp ; \
		sort -k3,4 $(ARCH_DIR)/boot/system.tmp > $(ARCH_DIR)/boot/system.map ; \
		rm -f $(ARCH_DIR)/boot/system.tmp )

Image: boot/system toolkit
	(cd boot; ../tools/imgconv system system.app -S192 -H72 -V)

# End SIBO image build

else

# Begin PC image build

boot/system:	$(XINCLUDE) $(AARCHIVES) $(ADRIVERS) boot/crt1.o boot/crt0.o
	(cd $(BASEDIR) ; $(LD) $(LDFLAGS) -M $(ARCH_LD) -T $(TOPDIR)/elks/elks-small.ld \
		$(ARCH_DIR)/boot/crt0.o $(ARCH_DIR)/boot/crt1.o \
		init/main.o '-(' $(ARCHIVES) $(DRIVERS) '-)' \
		-o $(ARCH_DIR)/boot/system > $(ARCH_DIR)/boot/system.map)
#		sort -k3,4 $(ARCH_DIR)/boot/system.tmp > $(ARCH_DIR)/boot/system.map ; \
#		rm -f $(ARCH_DIR)/boot/system.tmp )

ifneq ($(CONFIG_ROMCODE), y)

ifeq ($(CONFIG_286PMODE), y)

Image:	toolkit boot/bootsect boot/setup lib/lib86.a 286pmode/pmode286 boot/system
	tools/build boot/bootsect boot/setup 286pmode/pmode286 boot/system > Image

else

Image:	toolkit boot/bootsect boot/setup boot/system
	tools/build boot/bootsect boot/setup boot/system > boot/Image

nbImage:	Image boot/netbootsect
	tools/mknbi-elks boot/netbootsect boot/Image boot/nbImage

endif

else

ifeq ($(CONFIG_ROM_BOOTABLE_BY_RESET), y)
    RVECT = -r $(CONFIG_ROM_RESET_ADDRESS)   
endif

# Disable temporary the checksum to not overwrite the image
# as it is actually not needed for execution in EMU86

Image:	toolkit boot/setup boot/system  
#	tools/mkbootloader $(RVECT) -c $(CONFIG_ROM_SETUP_CODE) $(CONFIG_ROM_CHECKSUM_SIZE) boot/Image $(CONFIG_ROM_BASE) -a boot/setup $(CONFIG_ROM_SETUP_CODE) -s boot/system $(CONFIG_ROM_KERNEL_CODE) $(CONFIG_ROM_BIOS_MODULE) $(CONFIG_ROM_BIOS_MODULE_ADDR)
	tools/mkbootloader $(RVECT) boot/Image $(CONFIG_ROM_BASE) -a boot/setup $(CONFIG_ROM_SETUP_CODE) -s boot/system $(CONFIG_ROM_KERNEL_CODE) $(CONFIG_ROM_BIOS_MODULE) $(CONFIG_ROM_BIOS_MODULE_ADDR)
	$(CONFIG_ROM_SIMULATOR_PROGRAM)

# End PC image build

endif

#########################################################################
# Other commands.

boot:	Image
	dd if=boot/Image of=/dev/fd0 bs=8192

disk:	boot

setup:	toolkit boot/setup  
	tools/mkbootloader $(RVECT) -c $(CONFIG_ROM_SETUP_CODE) $(ROM_MAX_SETUP_SIZE) Image $(CONFIG_ROM_BASE) -a boot/setup $(CONFIG_ROM_SETUP_CODE) -s boot/system $(CONFIG_ROM_KERNEL_CODE) $(CONFIG_ROM_BIOS_MODULE) $(CONFIG_ROM_BIOS_MODULE_ADDR)
	$(CONFIG_ROM_SIMULATOR_PROGRAM)
endif

#########################################################################
# library rules

# Build archives and add them to the list of things to link into kernel image

.PHONY: kernel/akernel.a lib/lib86.a mm/mm.a drivers/char/chr_drv.a \
	drivers/block/blk_drv.a drivers/net/net_drv.a 286pmode/pmode286

kernel/akernel.a:
	${MAKE} -C kernel akernel.a

lib/lib86.a:
	${MAKE} -C lib lib86.a

mm/mm.a:
	${MAKE} -C mm mm.a

drivers/char/chr_drv.a:
	${MAKE} -C drivers/char chr_drv.a

drivers/block/blk_drv.a:
	${MAKE} -C drivers/block blk_drv.a

drivers/net/net_drv.a:
	${MAKE} -C drivers/net net_drv.a

286pmode/pmode286:
	${MAKE} -C 286pmode pmode286

#########################################################################
# commands.

distdir:
	mkdir -p $(DISTDIR)/arch/i86
	(cd $(DISTDIR)/arch/i86 ; \
		mkdir -p 286pmode boot drivers kernel lib mm tools)
	(cd $(DISTDIR)/arch/i86/drivers ; \
		mkdir -p block char)
	cp -pf defconfig Makefile $(DISTDIR)/arch/i86
	cp -pf boot/*.S boot/*.c boot/tux.pic $(DISTDIR)/arch/i86/boot
	cp -pf tools/Makefile tools/*.h $(DISTDIR)/arch/i86/tools
	cp -pf tools/mknbi-elks tools/*.c $(DISTDIR)/arch/i86/tools
	cp -pf tools/*.s tools/*.1 $(DISTDIR)/arch/i86/tools
	${MAKE} -C 286pmode distdir
	${MAKE} -C drivers/block distdir
	${MAKE} -C drivers/char distdir
	${MAKE} -C kernel distdir
	${MAKE} -C lib distdir
	${MAKE} -C mm distdir

#########################################################################
### Dependencies:
