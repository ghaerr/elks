# Makefile for compiling assembler for Psion 3a.		SDW 28/2/99
#
###########################################################################
#
# Note that the standard Makefile rules and defines have been moved to
# the file below.

ELKSDIR		= ../../..

include $(ELKSDIR)/Makefile-rules

###########################################################################

## Paramters
stack_size = 64		# paras of 16 bytes
heap_size = 128		# paras of 16 bytes

imgconv = ./imgconv
imgconv_flags = -D$(stack_size) -H$(heap_size)

%.img:%.out		# convert to psion
	$(imgconv) $< $@ 

## Targets
## NOTE you have to build the psion tree first to get driver stuff

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
	rm -f *.o
	rm -f *.out
	rm -f *.img
	rm -f imgconv mkurlader
