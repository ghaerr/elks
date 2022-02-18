# for CONFIG_ROMCODE
include $(TOPDIR)/.config

CC = ia16-elf-gcc
# See boot_minix.c on -fno-top-level-reorder.
CROSS_CFLAGS = -mregparmcall -fno-toplevel-reorder -fno-inline -mcmodel=tiny -mno-segment-relocation-stuff -ffreestanding -mtune=i8086
INCLUDES = -I $(TOPDIR)/include -I $(TOPDIR)/elks/include
CFLAGS = -Wall -Wno-strict-aliasing -Os $(CROSS_CFLAGS) $(INCLUDES)

AS = ia16-elf-as
# We do some assembly-time divisions in boot_sect_fat.h, so ask GNU as not to treat `/' as a comment character.
ASFLAGS = --divide

LD = ia16-elf-ld
LDFLAGS = -T $(TOPDIR)/elks/elks-raw.ld

.PHONY: all

TARGETS = mbr.bin

ifneq ($(CONFIG_ROMCODE), y)
TARGETS += minix.bin probe.bin fat.bin
endif

all: $(TARGETS)

minix.bin: boot_sect.o boot_minix.o
	$(LD) $(LDFLAGS) -M -o minix.bin boot_sect.o boot_minix.o > minix.map

probe.bin: boot_sect.o boot_probe.o
	$(LD) $(LDFLAGS) -M -o probe.bin boot_sect.o boot_probe.o > probe.map

boot_sect.o: boot_sect.S $(TOPDIR)/include/autoconf.h
	$(CC) $(INCLUDES) -E -o boot_sect.tmp boot_sect.S
	$(CC) $(INCLUDES) -E -UBOOT_FAT -o boot_sect.tmp boot_sect.S
	$(AS) $(ASFLAGS) -o boot_sect.o boot_sect.tmp
	rm -f boot_sect.tmp

boot_probe.o: boot_probe.S $(TOPDIR)/include/autoconf.h
	$(CC) $(INCLUDES) -E -o boot_probe.tmp boot_probe.S
	$(AS) $(ASFLAGS) -oboot_probe.o boot_probe.tmp
	rm -f boot_probe.tmp

boot_minix.o: boot_minix.c $(TOPDIR)/include/autoconf.h

fat.bin: boot_sect_fat.o $(TOPDIR)/include/autoconf.h
	$(LD) $(LDFLAGS) -M -o fat.bin boot_sect_fat.o > fat.map

mbr.bin: mbr.S
	$(CC) $(INCLUDES) -E -o mbr.tmp mbr.S
	$(AS) $(ASFLAGS) -o mbr.o mbr.tmp
	$(LD) $(LDFLAGS) -M -o mbr.bin mbr.o > mbr.map
	xxd -i mbr.bin > mbr_autogen.c
	rm -f mbr.tmp

boot_sect_fat.o: boot_sect.S boot_sect_fat.h $(TOPDIR)/include/autoconf.h
	$(CC) $(INCLUDES) -E -DBOOT_FAT -o boot_sect_fat.tmp boot_sect.S
	$(AS) $(ASFLAGS) -o boot_sect_fat.o boot_sect_fat.tmp
	rm -f boot_sect_fat.tmp

clean:
	rm -f *.o *.bin *.map mbr_autogen.c

# Boot blocks are not part of the target filesystem
# but embedded in the target disk image
# so empty rule for installation

install:
