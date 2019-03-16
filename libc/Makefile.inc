
VERSION=elks-0.3.0

INCLUDES=-I$(TOPDIR)/include -I$(TOPDIR)/libc/include -I$(TOPDIR)/elks/include
DEFINES=-D__LIBC__ -D__LIBC_VER__='"$(VERSION)"' -D__HAS_NO_FLOATS__

SDEFS=$(DEFINES)
CDEFS=$(DEFINES)

ARCH=-ffreestanding -fno-inline -melks -mcmodel=small -mno-segment-relocation-stuff -mtune=i8086

CC=ia16-elf-gcc
AS=ia16-elf-as
AR=ia16-elf-ar
LD=ia16-elf-ld

CFLAGS=$(ARCH) $(INCLUDES) $(CDEFS) -Wall -Os
ASFLAGS=-mtune=i8086
LDFLAGS=-mtune=i8086

LIBC=$(TOPDIR)/libc/libc.a
LIB_CPU=i86
LIB_OS=ELKS

.S.o: ;

.S.s:
	gcc -E -traditional $(INCS) $(SDEFS) -o $*.s $<

.s.o:
	$(AS) $(ASFLAGS) -o $*.o $<

.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<
