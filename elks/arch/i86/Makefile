# Build the Psion specific stuff

AS86	=as86 -0
LD86	=ld86 -0

AS	=as
LD	=ld86
ifeq ($(CONFIG_ARCH_SIBO), y)
LDFLAGS	=-0 -i -D 0xc00 -H 0x480
else
LDFLAGS	=-0 -i
endif
CC	=bcc

#########################################################################

AARCHIVES = kernel/akernel.a lib/lib86.a mm/mm.a

ARCHIVES := $(ARCHIVES) $(ARCH_DIR)/kernel/akernel.a \
	$(ARCH_DIR)/lib/lib86.a \
	$(ARCH_DIR)/mm/mm.a

ADRIVERS = drivers/char/chr_drv.a drivers/block/blk_drv.a

DRIVERS := $(DRIVERS) $(ARCH_DIR)/drivers/char/chr_drv.a \
	$(ARCH_DIR)/drivers/block/blk_drv.a

#########################################################################
# general construction rules

.c.s:
	$(CC) $(CFLAGS) \
	-0 -nostdinc -I$(MT_DIR)/include -S -o $*.s $<
.s.o:
	$(AS) -C -o -I$(MT_DIR)/include -o $*.o $<
.S.s:
	gcc -E -traditional -o $*.s $<
.c.o:
	$(CC) $(CFLAGS) \
	2 -nostdinc -I$(MT_DIR)/include -c -o $*.o $<

#########################################################################
# arch tools
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
	(cd tools; make)

tools/build: tools/build.c
	gcc -I $(TOPDIR)/include -o tools/build tools/build.c

tools/mkurlader: tools/mkurlader.c
	gcc -I $(TOPDIR)/include -o tools/mkurlader tools/mkurlader.c

ifeq ($(CONFIG_ARCH_SIBO), y)

boot/system: $(AARCHIVES) $(ADRIVERS) sibo/crt1.o sibo/crt0.o
	(cd ../.. ; $(LD) $(LDFLAGS) $(ARCH_DIR)/sibo/crt0.o \
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
	(cd ../.. ; $(LD) $(LDFLAGS) $(ARCH_DIR)/boot/crt0.o \
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

ifeq ($(ROM_RESET_VEC), y)
  RVECT = -r $(ROM_RESET_ADDR)   
endif

Image: tools boot/setup boot/system  
	tools/mkurlader $(RVECT) -c $(ROM_SETUP_CODE) $(ROM_CHECKSUM_SIZE) Image $(ROM_BASIS) -a boot/setup $(ROM_SETUP_CODE) -s boot/system $(ROM_KERNEL_CODE) $(ROM_EXT_MOD1) $(ROM_EXT_MOD1_ADDR)
	sync
	$(ROM_E_SIM_CALL)

endif
#-----


boot: Image
	dd if=boot/Image of=/dev/fd0 bs=8192

disk: boot

setup: $(ARCH_DIR)/tools $(ARCH_DIR)/boot/setup  
	tools/mkurlader $(RVECT) -c $(ROM_SETUP_CODE) $(ROM_MAX_SETUP_SIZE) Image $(ROM_BASIS) -a boot/setup $(ROM_SETUP_CODE) -s boot/system $(ROM_KERNEL_CODE) $(ROM_EXT_MOD1) $(ROM_EXT_MOD1_ADDR)
	sync
	$(ROM_E_SIM_CALL)
endif

#########################################################################
# library rules
# Build archives and add them to the list of things to link into kernel image
.PHONY: kernel/akernel.a lib/lib86.a mm/mm.a drivers/char/chr_drv.a \
	drivers/block/blk_drv.a 286pmode/pmode286

kernel/akernel.a:
	(cd kernel; make)

lib/lib86.a:
	(cd lib; make)

mm/mm.a:
	(cd mm; make)

drivers/char/chr_drv.a:
	(cd drivers/char; make)

drivers/block/blk_drv.a:
	(cd drivers/block; make)

286pmode/pmode286:
	(cd 286pmode; make)

#########################################################################
# misc
clean:
	rm -f boot/bootsect boot/setup boot/bootsect.s boot/setup.s
	rm -f boot/netbootsect boot/netbootsect.s
	rm -f boot/*~ boot/*.o boot/system boot/Image boot/system.app
	rm -f boot/nbImage boot/nbImage.rd Netboot.map
	rm -f tools/*~ tools/*.o tools/build
	rm -f Boot.map Setup.map boot/crt0.s
	(cd kernel; make clean)
	(cd lib; make clean)
	(cd mm; make clean)
	(cd drivers/block; make clean)
	(cd drivers/char; make clean)
	(cd 286pmode; make clean)

distdir:
	cp -pf config.in defconfig Makefile $(TOPDIR)/$(DISTDIR)/arch/i86
	(mkdir $(TOPDIR)/$(DISTDIR)/arch/i86/kernel; cd kernel; make distdir)
	(mkdir $(TOPDIR)/$(DISTDIR)/arch/i86/lib; cd lib; make distdir)
	(mkdir $(TOPDIR)/$(DISTDIR)/arch/i86/mm; cd mm; make distdir)
	(mkdir -p $(TOPDIR)/$(DISTDIR)/arch/i86/drivers/block; cd drivers/block; make distdir)
	(mkdir -p $(TOPDIR)/$(DISTDIR)/arch/i86/drivers/char; cd drivers/char; make distdir)
	(mkdir $(TOPDIR)/$(DISTDIR)/arch/i86/286pmode; cd 286pmode; make distdir)
	(mkdir $(TOPDIR)/$(DISTDIR)/arch/i86/boot)
	cp -pf boot/*.S boot/*.c boot/tux.pic $(TOPDIR)/$(DISTDIR)/arch/i86/boot
	(mkdir $(TOPDIR)/$(DISTDIR)/arch/i86/tools)
	cp -pf tools/Makefile tools/*.h $(TOPDIR)/$(DISTDIR)/arch/i86/tools
	cp -pf tools/mknbi-elks tools/*.c $(TOPDIR)/$(DISTDIR)/arch/i86/tools
	cp -pf tools/*.s tools/*.1 $(TOPDIR)/$(DISTDIR)/arch/i86/tools

dep:
	(cd kernel; make dep)
	(cd lib; make dep)
	(cd mm; make dep)
	(cd drivers/block; make dep)
	(cd drivers/char; make dep)
	(cd 286pmode; make dep)
