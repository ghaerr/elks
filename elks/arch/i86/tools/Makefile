# Makefile for compiling assembler for Psion 3a - SDW 28/2/99
#
###########################################################################
# Relative path to base directory.

BASEDIR 	= ../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC  	= Y

CLEANDEP	= 

CLEANME 	= build imgconv mkurlader wrt_disk.img

DEPEND  	= 

DISTFILES	= mknbi-elks

NOINDENT	= 

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

wrt_disk.tmp: wrt_disk.o file.o
	ld86 -0 -i -D $(DATA_SEG) -H $(HEAP_SEG) -o wrt_disk.tmp \
		wrt_disk.o file.o \
		../drivers/char/con_asm.o \
		../drivers/char/font.o \
		../drivers/char/key_asm.o \
		../drivers/block/ssd_asm.o

wrt_disk.img: wrt_disk.tmp imgconv
	./imgconv wrt_disk.tmp wrt_disk.img

build: build.c
	gcc -I $(BASEDIR)/include -o build build.c

mkurlader: mkurlader.c
	gcc -I $(BASEDIR)/include -o mkurlader mkurlader.c

imgconv: imgconv.c
	gcc -o imgconv imgconv.c

###########################################################################
### Dependencies:

build.o: build.c /usr/include/stdio.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h \
 /usr/include/bits/types.h /usr/include/libio.h \
 /usr/include/_G_config.h /usr/include/bits/stdio_lim.h \
 /usr/include/string.h /usr/include/stdlib.h /usr/include/sys/types.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/alloca.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/unistd.h \
 /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
 /usr/include/getopt.h /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 a.out.h /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h


disfp.o: disfp.c dis.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
 /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
 /usr/include/getopt.h /usr/include/string.h /usr/include/stdlib.h \
 /usr/include/alloca.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h img.h a.out.h ansi.h


dishand.o: dishand.c dis.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
 /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
 /usr/include/getopt.h /usr/include/string.h /usr/include/stdlib.h \
 /usr/include/alloca.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h img.h a.out.h ansi.h


dismain.o: dismain.c dis.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
 /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
 /usr/include/getopt.h /usr/include/string.h /usr/include/stdlib.h \
 /usr/include/alloca.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h img.h a.out.h ansi.h


disrel.o: disrel.c


distabs.o: distabs.c dis.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
 /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
 /usr/include/getopt.h /usr/include/string.h /usr/include/stdlib.h \
 /usr/include/alloca.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h img.h a.out.h ansi.h


getopt.o: getopt.c /usr/include/string.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h


imgconv.o: imgconv.c /usr/include/sys/types.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
 /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/sys/stat.h /usr/include/bits/stat.h /usr/include/fcntl.h \
 /usr/include/bits/fcntl.h /usr/include/unistd.h \
 /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
 /usr/include/getopt.h /usr/include/stdlib.h /usr/include/alloca.h \
 /usr/include/stdio.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/ctype.h img.h a.out.h


imgdump.o: imgdump.c /usr/include/stdio.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h \
 /usr/include/bits/types.h /usr/include/libio.h \
 /usr/include/_G_config.h /usr/include/bits/stdio_lim.h \
 /usr/include/sys/types.h /usr/include/time.h /usr/include/endian.h \
 /usr/include/bits/endian.h /usr/include/sys/select.h \
 /usr/include/bits/select.h /usr/include/bits/sigset.h \
 /usr/include/sys/sysmacros.h /usr/include/sys/stat.h \
 /usr/include/bits/stat.h /usr/include/fcntl.h \
 /usr/include/bits/fcntl.h /usr/include/unistd.h \
 /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
 /usr/include/getopt.h /usr/include/stdlib.h /usr/include/alloca.h \
 /usr/include/string.h /usr/include/ctype.h img.h


makelist.o: makelist.c /usr/include/stdio.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h \
 /usr/include/bits/types.h /usr/include/libio.h \
 /usr/include/_G_config.h /usr/include/bits/stdio_lim.h \
 /usr/include/string.h


mkurlader.o: mkurlader.c /usr/include/stdio.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h \
 /usr/include/bits/types.h /usr/include/libio.h \
 /usr/include/_G_config.h /usr/include/bits/stdio_lim.h \
 /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
 /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/alloca.h /usr/include/string.h


writedisk.o: writedisk.c


wrt_disk.o: wrt_disk.c


