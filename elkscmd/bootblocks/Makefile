
CC = ia16-elf-gcc
CROSS_CFLAGS = -mregparmcall -fno-toplevel-reorder -fno-inline -mcmodel=tiny -mno-segment-relocation-stuff -ffreestanding -mtune=i8086
INCLUDES = -I $(TOPDIR)/include
CFLAGS = -Wall -Os $(CROSS_CFLAGS) $(INCLUDES)

AS = ia16-elf-as
ASFLAGS =

LD = ia16-elf-ld
LDFLAGS = -T $(TOPDIR)/elks/elks-raw.ld

.PHONY: all

all: minix.bin probe.bin

minix.bin: boot_sect.o boot_minix.o
	$(LD) $(LDFLAGS) -M -o minix.bin boot_sect.o boot_minix.o > minix.map

probe.bin: boot_sect.o boot_probe.o
	$(LD) $(LDFLAGS) -M -o probe.bin boot_sect.o boot_probe.o > probe.map

boot_sect.o: boot_sect.S
	$(CC) $(INCLUDES) -E -o boot_sect.tmp boot_sect.S
	$(AS) -o boot_sect.o boot_sect.tmp
	rm -f boot_sect.tmp

boot_probe.o: boot_probe.S
	$(CC) $(INCLUDES) -E -o boot_probe.tmp boot_probe.S
	$(AS) -oboot_probe.o boot_probe.tmp
	rm -f boot_probe.tmp

boot_minix.o: boot_minix.c

clean realclean:
	rm -f *.o *.bin

# Boot blocks are not part of the target filesystem
# but embedded in the target disk image
# so empty rule for installation

install:
