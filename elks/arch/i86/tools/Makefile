#
#
###########################################################################
# Relative path to base directory.

BASEDIR 	= ../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC  	= N

CLEANDEP	= 

CLEANME 	= build mkbootloader

DEPEND  	= 

DISTFILES	= 

NOINDENT	= 

###########################################################################
# Include the standard rules.

include $(BASEDIR)/Makefile-rules

###########################################################################
# Targets. You have to build the psion tree first to get driver stuff.

all: build mkbootloader

build: build.c
	gcc $(INCLUDES) -o build build.c

mkbootloader: mkbootloader.c
	gcc $(INCLUDES) -o mkbootloader mkbootloader.c

# SIBO stuff
DATA_SEG	= 0x400 	# Segment address
HEAP_SEG	= 0x800 	# Segment address
wrt_disk.o: wrt_disk.c

wrt_disk.tmp: wrt_disk.o file.o
	ld86 -0 -i -D $(DATA_SEG) -H $(HEAP_SEG) -o wrt_disk.tmp \
		wrt_disk.o file.o \
		../drivers/char/con_asm.o \
		../drivers/char/font.o \
		../drivers/char/key_asm.o \
		../drivers/block/ssd_asm.o \
		../lib/lib86.a

wrt_disk.img: wrt_disk.tmp imgconv
	./imgconv wrt_disk.tmp wrt_disk.img

imgconv: imgconv.c
	gcc -o imgconv imgconv.c

###########################################################################
### Dependencies:
