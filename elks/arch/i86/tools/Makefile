# Makefile for compiling assembler for Psion 3a - SDW 28/2/99
#
###########################################################################
# Relative path to base directory.

BASEDIR 	= ../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC		= Y

CLEANDEP	= 

CLEANME 	= build imgconv mkurlader wrt_disk.img

DEPEND		= 

DISTFILES	= mknbi-elks

###########################################################################
# Include the standard rules.

include $(BASEDIR)/Makefile-rules

###########################################################################
# SIBO paramters.

STACK_SIZE	= 64		# paragraphs of 16 bytes
HEAP_SIZE	= 128		# paragraphs of 16 bytes

DATA_SEG	= 0x400 	# Segment address
HEAP_SEG	= 0x800 	# Segment address

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
	ld86 -0 -i -D $(DATA_SEG) -H $(HEAP_SEG) -o wrt_disk.out \
		wrt_disk.o \
		file.o \
		../drivers/char/con_asm.o \
		../drivers/char/font.o \
		../drivers/char/key_asm.o \
		../drivers/block/ssd_asm.o

wrt_disk.img: wrt_disk.out imgconv
	./imgconf -D$(STACK_SIZE) -H$(HEAP_SIZE)

build: build.c
	gcc -I $(BASEDIR)/include -o build build.c

mkurlader: mkurlader.c
	gcc -I $(BASEDIR)/include -o mkurlader mkurlader.c

imgconv: imgconv.c
	gcc -o imgconv imgconv.c

###########################################################################
### Dependencies:
