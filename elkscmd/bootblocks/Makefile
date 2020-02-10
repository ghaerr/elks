
CC = ia16-elf-gcc
# See boot_minix.c on -fno-top-level-reorder.
CROSS_CFLAGS = -mregparmcall -fno-toplevel-reorder -fno-inline -mcmodel=tiny -mno-segment-relocation-stuff -ffreestanding -mtune=i8086
INCLUDES = -I $(TOPDIR)/include
CFLAGS = -Wall -Os $(CROSS_CFLAGS) $(INCLUDES)

AS = ia16-elf-as
ASFLAGS =

LD = ia16-elf-ld
LDFLAGS = -T $(TOPDIR)/elks/elks-raw.ld

.PHONY: all

all: minix.bin probe.bin fat.bin

minix.bin: boot_sect.o boot_minix.o
	$(LD) $(LDFLAGS) -M -o minix.bin boot_sect.o boot_minix.o > minix.map

probe.bin: boot_sect.o boot_probe.o
	$(LD) $(LDFLAGS) -M -o probe.bin boot_sect.o boot_probe.o > probe.map

boot_sect.o: boot_sect.S
	$(CC) $(INCLUDES) -E -o boot_sect.tmp boot_sect.S
	$(CC) $(INCLUDES) -E -UBOOT_FAT -o boot_sect.tmp boot_sect.S
	$(AS) $(ASFLAGS) -o boot_sect.o boot_sect.tmp
	rm -f boot_sect.tmp

boot_probe.o: boot_probe.S
	$(CC) $(INCLUDES) -E -o boot_probe.tmp boot_probe.S
	$(AS) $(ASFLAGS) -oboot_probe.o boot_probe.tmp
	rm -f boot_probe.tmp

boot_minix.o: boot_minix.c

fat.bin: boot_sect_fat.o
	$(LD) $(LDFLAGS) -M -o fat.bin boot_sect_fat.o > fat.map

boot_sect_fat.o: boot_sect.S boot_sect_fat.h
	$(CC) $(INCLUDES) -E -DBOOT_FAT -o boot_sect_fat.tmp boot_sect.S
	$(AS) $(ASFLAGS) -o boot_sect_fat.o boot_sect_fat.tmp
	rm -f boot_sect_fat.tmp

clean realclean:
	rm -f *.o *.bin

# Boot blocks are not part of the target filesystem
# but embedded in the target disk image
# so empty rule for installation

install:
