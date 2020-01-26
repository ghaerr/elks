
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

minix.bin: minix_first.o minix_second.o
	$(LD) $(LDFLAGS) -M -o minix.bin minix_first.o minix_second.o > minix.map

minix_first.o: minix_first.S
	$(CC) $(INCLUDES) -E -o minix_first.tmp minix_first.S
	$(AS) -o minix_first.o minix_first.tmp
	rm -f minix_first.tmp

minix_second.o: minix_second.c

clean realclean:
	rm -f *.o *.bin

# Boot blocks are not part of the target filesystem
# but embedded in the target disk image
# so empty rule for installation

install:
