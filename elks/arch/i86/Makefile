# Note that the standard Makefile rules and defines have been moved to
# the file below.

ELKSDIR		= ../..

include $(ELKSDIR)/Makefile-rules

#########################################################################
# Build the Psion specific stuff

AARCHIVES = kernel/akernel.a lib/lib86.a mm/mm.a

ARCHIVES := $(ARCHIVES) $(ARCH_DIR)/kernel/akernel.a \
	$(ARCH_DIR)/lib/lib86.a \
	$(ARCH_DIR)/mm/mm.a

ADRIVERS = drivers/char/chr_drv.a drivers/block/blk_drv.a

DRIVERS := $(DRIVERS) $(ARCH_DIR)/drivers/char/chr_drv.a \
	$(ARCH_DIR)/drivers/block/blk_drv.a

#########################################################################
# arch tools

.PHONY: boot/system tools

boot/setup: boot/setup.S
	gcc -E -traditional -I$(TOPDIR)/include/ -o boot/setup.s boot/setup.S
	$(AS) $(AFLAGS) -o boot/setup.o boot/setup.s
	$(LD) $(LDFLAGS) -s -o boot/setup -M boot/setup.o > Setup.map

boot/bootsect: boot/bootsect.S
	gcc -E -traditional -I$(TOPDIR)/include/ -o boot/bootsect.s \
		boot/bootsect.S
	$(AS) $(AFLAGS) -o boot/bootsect.o boot/bootsect.s
	$(LD) $(LDFLAGS) -s -o boot/bootsect -M boot/bootsect.o > Boot.map

boot/netbootsect: boot/netbootsect.S
	gcc -E -traditional -I$(TOPDIR)/include/ -o boot/netbootsect.s \
		boot/netbootsect.S
	$(AS) $(AFLAGS) -o boot/netbootsect.o boot/netbootsect.s
	$(LD) $(LDFLAGS) -s -o boot/netbootsect -M boot/netbootsect.o > Netboot.map

boot/crt1.o: boot/crt1.c

boot/crt0.o: boot/crt0.S
	gcc -E -traditional -I$(TOPDIR)/include/ -o boot/crt0.s boot/crt0.S
	$(AS) $(AFLAGS) -o boot/crt0.o boot/crt0.s

tools:
	make -C tools all

tools/build: tools/build.c
	gcc -I $(TOPDIR)/include -o tools/build tools/build.c

tools/mkurlader: tools/mkurlader.c
	gcc -I $(TOPDIR)/include -o tools/mkurlader tools/mkurlader.c

ifeq ($(CONFIG_ARCH_SIBO), y)

boot/system: $(AARCHIVES) $(ADRIVERS) sibo/crt1.o sibo/crt0.o
	(cd $(ELKSDIR); \
	$(LD) $(LDFLAGS) $(ARCH_DIR)/sibo/crt0.o $(ARCH_DIR)/sibo/crt1.o \
		init/main.o $(ARCHIVES) $(DRIVERS) \
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
#-----


boot: Image
	dd if=boot/Image of=/dev/fd0 bs=8192

disk: boot

setup: $(ARCH_DIR)/tools $(ARCH_DIR)/boot/setup  
	tools/mkurlader $(RVECT) -c $(CONFIG_ROM_SETUP_CODE) $(ROM_MAX_SETUP_SIZE) Image $(CONFIG_ROM_BASE) -a boot/setup $(CONFIG_ROM_SETUP_CODE) -s boot/system $(CONFIG_ROM_KERNEL_CODE) $(CONFIG_ROM_BIOS_MODULE) $(CONFIG_ROM_BIOS_MODULE_ADDR)
	sync
	$(CONFIG_ROM_SIMULATOR_PROGRAM)
endif

#########################################################################
# library rules to build archives and add them to the list of things to
# link into kernel image

.PHONY: kernel/akernel.a lib/lib86.a mm/mm.a drivers/char/chr_drv.a \
	drivers/block/blk_drv.a 286pmode/pmode286

286pmode/pmode286:
	make -C 286pmode all

drivers/block/blk_drv.a:
	make -C drivers/block all

drivers/char/chr_drv.a:
	make -C drivers/char all

kernel/akernel.a:
	make -C kernel all

lib/lib86.a:
	make -C lib all

mm/mm.a:
	make -C mm all

#########################################################################
# misc

clean:
	rm -f boot/bootsect boot/setup boot/bootsect.s boot/setup.s
	rm -f boot/netbootsect boot/netbootsect.s
	rm -f boot/*~ boot/*.o boot/system boot/Image boot/system.app
	rm -f boot/nbImage boot/nbImage.rd Netboot.map
	rm -f tools/*~ tools/*.o tools/build
	rm -f Boot.map Setup.map boot/crt0.s
	make -C 286pmode clean
	make -C drivers clean
	make -C drivers/block clean
	make -C drivers/char clean
	make -C kernel clean
	make -C lib clean
	make -C mm clean

distdir:
	cp -pf config.in defconfig Makefile $(TOPDIR)/$(DISTDIR)/arch/i86
	mkdir $(DISTDIR)/arch/i86/286pmode
	make -C 286pmode distdir
	mkdir -p $(DISTDIR)/arch/i86/drivers/block
	make -C drivers/block distdir
	mkdir -p $(DISTDIR)/arch/i86/drivers/char
	make -C drivers/char distdir
	mkdir $(DISTDIR)/arch/i86/kernel
	make -C kernel distdir
	mkdir $(DISTDIR)/arch/i86/lib
	make -C lib distdir
	mkdir $(DISTDIR)/arch/i86/mm
	make -C mm distdir
	mkdir $(DISTDIR)/arch/i86/boot
	cp -pf boot/*.S boot/*.c boot/tux.pic $(DISTDIR)/arch/i86/boot
	mkdir $(DISTDIR)/arch/i86/tools
	cp -pf tools/Makefile tools/*.h $(DISTDIR)/arch/i86/tools
	cp -pf tools/mknbi-elks tools/*.c $(DISTDIR)/arch/i86/tools
	cp -pf tools/*.s tools/*.1 $(DISTDIR)/arch/i86/tools

dep:
	make -C 286pmode dep
	make -C drivers/block dep
	make -C drivers/char dep
	make -C kernel dep
	make -C lib dep
	make -C mm dep

#########################################################################
### Dependencies:
