
CC=bcc
CFLAGS=-ansi -Ms -Oi -O
AS=as86
ASFLAGS=-0 -w
LDFLAGS=-s -i -H0x10000

MINIXDEFS=-DDOTS
LST=-l $*.lst

.PHONY: all

all: minix.bin minix_elks.bin

minix.bin: minix.s
minix_elks.bin: minix_elks.v

minix.s: minix.c Makefile
	$(CC) -Mf -O -DTRY_FLOPPY $(MINIXDEFS) -S minix.c

minix_elks.s: minix_elks.c Makefile minix.v
	$(CC) -Mf -O $(MINIXDEFS) -S minix_elks.c

clean realclean:
	rm -f minix.s minix_elks.s
	rm -f *.bin *.lst *.sym *.v

.SUFFIXES: .bin .v

.s.bin:
	$(AS) -w- -0 -b $*.bin -s $*.sym $*.s $(LST)

.s.v:
	./as86_encap $*.s $*.v $*_ $(ASFLAGS) $(LST)

# Boot blocks are not part of the target filesystem
# but embedded in the target disk image
# so empty rule for installation

install:
