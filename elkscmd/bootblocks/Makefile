
CC = ia16-elf-gcc
CROSS_CFLAGS = -fno-inline -mcmodel=tiny -mno-segment-relocation-stuff -ffreestanding -mtune=i8086
INCLUDES = -I $(TOPDIR)/include
CFLAGS = -Wall -Os $(CROSS_CFLAGS) $(INCLUDES)

AS = ia16-elf-as
ASFLAGS =

LD = ia16-elf-ld
LDFLAGS = -T $(TOPDIR)/elks/elks-raw.ld

.PHONY: all

all: minix.bin

minix.bin: boot_sect.o boot_minix.o
	$(LD) $(LDFLAGS) -M -o minix.bin boot_sect.o boot_minix.o > minix.map

boot_sect.o: boot_sect.S
	$(CC) $(INCLUDES) -E -o boot_sect.tmp boot_sect.S
	$(AS) -o boot_sect.o boot_sect.tmp
	rm -f boot_sect.tmp

boot_minix.o: boot_minix.c

clean realclean:
	rm -f *.o *.bin

# Boot blocks are not part of the target filesystem
# but embedded in the target disk image
# so empty rule for installation

install:
