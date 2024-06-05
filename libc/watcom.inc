# Makefile include for OpenWatcom build

ifeq "$(WATDIR)" ""
$(error Must set WATDIR environment variable to top of OpenWatcom C directory)
endif

INCLUDES = -I$(TOPDIR)/libc/include -I$(TOPDIR)/elks/include
INCLUDES += -I$(WATDIR)/bld/hdr/dos/h
DEFINES = -D__LIBC__
LIBOBJS=$(OBJS:.o=.obj)

include $(TOPDIR)/libc/watcom.model

CARCH =\
    -bos2                           \
    -mcmodel=$(MODEL)               \
    -march=i86                      \
    -std=c99                        \
    -fno-stack-check                \
    -Wc,-wcd=303                    \
    -Wc,-fpi87                      \
    -Wc,-zev                        \
    -fnostdlib                      \
    -v                              \

AARCH=\
    -m$(MODEL)                      \
    -0                              \
    -fp0                            \
    -fpi87                          \

CC=owcc
AR=wlib
AS=wasm

CFLAGS=$(CARCH) $(INCLUDES) $(DEFINES) -Wall -Os
ASFLAGS=$(AARCH)
ARFLAGS_SUB=-n -b -fo

%.obj: %.c
	$(CC) $(CFLAGS) -c -o $*.obj $<

%.obj: %.asm
	$(AS) $(ASFLAGS) $*.asm
	mv $*.o $*.obj
