# Makefile for compiling assembler for Psion 3a - SDW 28/2/99
#
###########################################################################
# Relative path to base directory.

ELKSDIR 	= ../../..

###########################################################################
# Paramters.

stack_size	= 64		# paragraphs of 16 bytes
heap_size	= 128		# paragraphs of 16 bytes

###########################################################################
# Standard commands.

AS		= as86
ASFLAGS 	= -0

CC		= bcc
CFLAGS		= -0 -c

IC		= ./imgconf
ICFLAGS		= -D$(stack_size) -H$(heap_size)

###########################################################################
# Standard prototypes.

.c.o:
	$(CC) $(CFLAGS) -o $@ $<

.s.o:
	$(AS) $(ASFLAGS) -o $@ $<

.out.img:
	$(IC) $(ICFLAGS) $< $@ 

###########################################################################
# Targets. You have to build the psion tree first to get driver stuff.

ifeq ($(CONFIG_ARCH_SIBO), y)

all: wrt_disk.img build mkurlader imgconv

else

all: build mkurlader

endif

wrt_disk.o: wrt_disk.c

file.o: file.s

wrt_disk.out: wrt_disk.o file.o
	ld86 -o wrt_disk.out -0 -i -D 0x400 -H 0x800 wrt_disk.o file.o \
		../drivers/char/con_asm.o \
		../drivers/char/font.o \
		../drivers/char/key_asm.o \
		../drivers/block/ssd_asm.o

wrt_disk.img: wrt_disk.out imgconv

build: build.c
	gcc -I $(TOPDIR)/include -o build build.c

mkurlader: mkurlader.c
	gcc -I $(TOPDIR)/include -o mkurlader mkurlader.c

imgconv: imgconv.c
	gcc -o imgconv imgconv.c

clean:
	rm -f *.o *.out *.img imgconv mkurlader

dep:
	@echo -n

###########################################################################
## Dependencies:
