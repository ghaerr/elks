# Build the Psion specific stuff.
#
#########################################################################
# Relative path to base directory.

ELKSDIR 	= ../..

#########################################################################
# SIBO specific options.

ifeq ($(CONFIG_ARCH_SIBO), y)

SIBOFLAGS 	= -D 0xc00 -H 0x480

else

SIBOFLAGS	=

endif

#########################################################################
# Standard commands.

AS86		= as86 -0
LD86		= ld86 -0

AS		= as
CC		= bcc
LD		= ld86

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
# Standard prototypes.

.c.s:
	$(CC) $(CFLAGS) -0 -nostdinc -I$(TOPDIR)/include -S -o $*.s $<
.s.o:
	$(AS) -C -o -I$(TOPDIR)/include -o $*.o $<
.S.s:
	gcc -E -traditional -o $*.s $<
.c.o:
	$(CC) $(CFLAGS) \
	2 -nostdinc -I$(TOPDIR)/include -c -o $*.o $<

#########################################################################
# Commands.

.PHONY: boot/system tools

boot/setup: boot/setup.S
	gcc -E -traditional -I$(TOPDIR)/include/ -o boot/setup.s boot/setup.S
	$(AS86) -o boot/setup.o boot/setup.s
	$(LD86) -s -o boot/setup -M boot/setup.o > Setup.map

boot/bootsect: boot/bootsect.S
	gcc -E -traditional -I$(TOPDIR)/include/ -o boot/bootsect.s \
		boot/bootsect.S
	$(AS86) -0 -o boot/bootsect.o boot/bootsect.s
	$(LD86) -0 -s -o boot/bootsect -M boot/bootsect.o > Boot.map

boot/netbootsect: boot/netbootsect.S
	gcc -E -traditional -I$(TOPDIR)/include/ -o boot/netbootsect.s \
		boot/netbootsect.S
	$(AS86) -0 -o boot/netbootsect.o boot/netbootsect.s
	$(LD86) -0 -s -o boot/netbootsect -M boot/netbootsect.o > Netboot.map

boot/crt1.o: boot/crt1.c

boot/crt0.o: boot/crt0.S
	gcc -E -traditional -I$(TOPDIR)/include/ -o boot/crt0.s boot/crt0.S
	$(AS86) -0 -o boot/crt0.o boot/crt0.s

tools:
	make -C tools

tools/build: tools/build.c
	gcc -I $(TOPDIR)/include -o tools/build tools/build.c

tools/mkurlader: tools/mkurlader.c
	gcc -I $(TOPDIR)/include -o tools/mkurlader tools/mkurlader.c

#########################################################################
# Image selection.

ifeq ($(CONFIG_ARCH_SIBO), y)

boot/system: $(AARCHIVES) $(ADRIVERS) sibo/crt1.o sibo/crt0.o
	(cd $(ELKSDIR) ; $(LD) $(LDFLAGS) $(ARCH_DIR)/sibo/crt0.o \
		$(ARCH_DIR)/sibo/crt1.o \
		init/main.o \
		$(ARCHIVES) \
		$(DRIVERS) \
		-t -M -o $(ARCH_DIR)/boot/system > System.map)

#SIBO image build
Image: boot/system tools
	(cd boot; ../tools/imgconv system system.app -S192 -H72 -V)

#End SIBO image section

else

boot/system: $(AARCHIVES) $(ADRIVERS) boot/crt1.o boot/crt0.o
	(cd $(ELKSDIR) ; $(LD) $(LDFLAGS) $(ARCH_DIR)/boot/crt0.o \
		$(ARCH_DIR)/boot/crt1.o \
		init/main.o \
		$(ARCHIVES) \
		$(DRIVERS) \
		-t -M -o $(ARCH_DIR)/boot/system > System.map)

#PC image build

ifneq ($(CONFIG_ROMCODE), y)

ifeq ($(CONFIG_286PMODE), y)

Image: tools/build boot/bootsect boot/setup 286pmode/pmode286 boot/system tools
	tools/build boot/bootsect boot/setup 286pmode/pmode286 \
		boot/system $(ROOT_DEV) > Image
	sync

else

Image: boot/bootsect boot/setup boot/system tools
	tools/build boot/bootsect boot/setup boot/system \
		$(ROOT_DEV) > boot/Image
	sync

nbImage: Image boot/netbootsect
	tools/mknbi-elks boot/netbootsect boot/Image boot/nbImage

endif

else

ifeq ($(CONFIG_ROM_BOOTABLE_BY_RESET), y)
  RVECT = -r $(CONFIG_ROM_RESET_ADDRESS)   
endif

Image: tools boot/setup boot/system  
	tools/mkurlader $(RVECT) -c $(CONFIG_ROM_SETUP_CODE) $(CONFIG_ROM_CHECKSUM_SIZE) Image $(CONFIG_ROM_BASE) -a boot/setup $(CONFIG_ROM_SETUP_CODE) -s boot/system $(CONFIG_ROM_KERNEL_CODE) $(CONFIG_ROM_BIOS_MODULE) $(CONFIG_ROM_BIOS_MODULE_ADDR)
	sync
	$(CONFIG_ROM_SIMULATOR_PROGRAM)

endif

#########################################################################
# Other commands.

boot: Image
	dd if=boot/Image of=/dev/fd0 bs=8192

disk: boot

setup: $(ARCH_DIR)/tools $(ARCH_DIR)/boot/setup  
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
	make -C drivers/char 

drivers/block/blk_drv.a:
	make -C drivers/block

286pmode/pmode286:
	make -C 286pmode

#########################################################################
# commands.

clean:
	rm -f boot/bootsect boot/setup boot/bootsect.s boot/setup.s
	rm -f boot/netbootsect boot/netbootsect.s
	rm -f boot/*~ boot/*.o boot/system boot/Image boot/system.app
	rm -f boot/nbImage boot/nbImage.rd Netboot.map
	rm -f tools/*~ tools/*.o tools/build
	rm -f Boot.map Setup.map boot/crt0.s
	make -C 286pmode clean
	make -C drivers/block clean
	make -C drivers/char clean
	make -C kernel clean
	make -C lib clean
	make -C mm clean

distdir:
#	mkdir $(TOPDIR)/$(DISTDIR)/arch/i86
	(cd $(TOPDIR)/$(DISTDIR)/arch/i86 ; \
		mkdir 286pmode boot drivers kernel lib mm tools)
	(cd $(TOPDIR)/$(DISTDIR)/arch/i86/drivers ; \
		mkdir block char)
	cp -pf config.in defconfig Makefile $(TOPDIR)/$(DISTDIR)/arch/i86
	cp -pf boot/*.S boot/*.c boot/tux.pic $(TOPDIR)/$(DISTDIR)/arch/i86/boot
	cp -pf tools/Makefile tools/*.h $(TOPDIR)/$(DISTDIR)/arch/i86/tools
	cp -pf tools/mknbi-elks tools/*.c $(TOPDIR)/$(DISTDIR)/arch/i86/tools
	cp -pf tools/*.s tools/*.1 $(TOPDIR)/$(DISTDIR)/arch/i86/tools
	make -C 286pmode distdir
	make -C drivers/block distdir
	make -C drivers/char distdir
	make -C kernel distdir
	make -C lib distdir
	make -C mm distdir

dep:
	make -C 286pmode dep
	make -C drivers/block dep
	make -C drivers/char dep
	make -C kernel dep
	make -C lib dep
	make -C mm dep

#########################################################################
### Dependencies:
