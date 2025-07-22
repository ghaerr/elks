#
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
# ROM specific options.

ifeq ($(CONFIG_ROMCODE), y)

# Make sure that the a.out header & data segment values are prefixed with `0x'
ROMPOSTLINKFLAGS = --aout-seg 0x$(CONFIG_ROM_KERNEL_CODE:0x%=%) \
		   --data-seg 0x$(CONFIG_ROM_KERNEL_DATA:0x%=%)

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
# Path to GCC's compiler support routines (libgcc.a).

# Ask GCC for the correct path to the library given our compiler options

LIBGCC = "`$(CC) $(CFLAGS) -print-libgcc-file-name`"

#########################################################################
# Things to make.

.PHONY: toolkit
toolkit:
	${MAKE} -C tools all

boot/setup:   boot/setup.o
	$(LD) $(LDFLAGS) -M -T $(TOPDIR)/elks/elks-tiny.ld -o boot/setup boot/setup.o > boot/setup.map
	$(POSTLINK) --tiny boot/setup

$(BASEDIR)/include/arch/asm-offsets.h: kernel/asm-offsets.c asm-offsets.awk
	$(CC) $(CFLAGS) -S -o asm-offsets.s kernel/asm-offsets.c
	awk -f asm-offsets.awk asm-offsets.s > $(BASEDIR)/include/arch/asm-offsets.h
	rm asm-offsets.s

#########################################################################
# Image selection.

# Begin PC image build

boot/system:	$(XINCLUDE) $(AARCHIVES) $(ADRIVERS) boot/crt0.o
	(cd $(BASEDIR) ; $(LD) $(CPU_LD) -M $(ARCH_LD) -T $(TOPDIR)/elks/elks-small.ld \
		$(ARCH_DIR)/boot/crt0.o \
		init/main.o '-(' $(ARCHIVES) $(DRIVERS) '-)' $(LIBGCC) \
		-o $(ARCH_DIR)/boot/system > $(ARCH_DIR)/boot/system-full.map ; \
		$(NM) $(ARCH_DIR)/boot/system | sed -e '/&/d; /!/d' | sort > \
			$(ARCH_DIR)/boot/system.map; \
		$(POSTLINK) --symfile $(ARCH_DIR)/boot/system.sym $(ARCH_DIR)/boot/system)

ifneq ($(CONFIG_ROMCODE), y)

Image:	toolkit boot/setup boot/system
	tools/build arg_ignored boot/setup boot/system > boot/Image

else

ifeq ($(CONFIG_ROM_BOOTABLE_BY_RESET), y)
    RVECT = -r $(CONFIG_ROM_RESET_ADDRESS)   
endif

ifdef CONFIG_ARCH_SWAN
    SWANARG = -w
endif

Image:	toolkit boot/setup boot/system  
	tools/mkbootloader $(SWANARG) $(RVECT) -c $(CONFIG_ROM_SETUP_CODE) $(CONFIG_ROM_CHECKSUM_SIZE) boot/Image $(CONFIG_ROM_BASE) -a boot/setup $(CONFIG_ROM_SETUP_CODE) -s boot/system $(CONFIG_ROM_KERNEL_CODE) $(CONFIG_ROM_BIOS_MODULE) $(CONFIG_ROM_BIOS_MODULE_ADDR)
	$(CONFIG_ROM_SIMULATOR_PROGRAM)

# End PC image build

endif

#########################################################################
# Other commands.

setup:	toolkit boot/setup  
	tools/mkbootloader $(RVECT) -c $(CONFIG_ROM_SETUP_CODE) $(ROM_MAX_SETUP_SIZE) Image $(CONFIG_ROM_BASE) -a boot/setup $(CONFIG_ROM_SETUP_CODE) -s boot/system $(CONFIG_ROM_KERNEL_CODE) $(CONFIG_ROM_BIOS_MODULE) $(CONFIG_ROM_BIOS_MODULE_ADDR)
	$(CONFIG_ROM_SIMULATOR_PROGRAM)

#########################################################################
# library rules

# Build archives and add them to the list of things to link into kernel image

.PHONY: kernel/akernel.a lib/lib86.a mm/mm.a drivers/char/chr_drv.a \
	drivers/block/blk_drv.a drivers/net/net_drv.a

kernel/akernel.a: $(XINCLUDE)
	${MAKE} -C kernel akernel.a

lib/lib86.a:
	${MAKE} -C lib lib86.a

mm/mm.a: $(XINCLUDE)
	${MAKE} -C mm mm.a

drivers/char/chr_drv.a:
	${MAKE} -C drivers/char chr_drv.a

drivers/block/blk_drv.a:
	${MAKE} -C drivers/block blk_drv.a

drivers/net/net_drv.a: $(XINCLUDE)
	${MAKE} -C drivers/net net_drv.a

#########################################################################
# commands.

distdir:
	mkdir -p $(DISTDIR)/arch/i86
	(cd $(DISTDIR)/arch/i86 ; \
		mkdir -p boot drivers kernel lib mm tools)
	(cd $(DISTDIR)/arch/i86/drivers ; \
		mkdir -p block char)
	cp -pf defconfig Makefile $(DISTDIR)/arch/i86
	cp -pf boot/*.S boot/*.c $(DISTDIR)/arch/i86/boot
	cp -pf tools/Makefile tools/*.h $(DISTDIR)/arch/i86/tools
	cp -pf tools/mknbi-elks tools/*.c $(DISTDIR)/arch/i86/tools
	cp -pf tools/*.s tools/*.1 $(DISTDIR)/arch/i86/tools
	${MAKE} -C drivers/block distdir
	${MAKE} -C drivers/char distdir
	${MAKE} -C kernel distdir
	${MAKE} -C lib distdir
	${MAKE} -C mm distdir

#########################################################################
### Dependencies:
