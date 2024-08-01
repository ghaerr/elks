# Makefile include for OpenWatcom build

ifeq "$(WATCOM)" ""
$(error WATCOM environment variable not set)
endif

INCLUDES = -I$(TOPDIR)/libc/include -I$(TOPDIR)/elks/include
INCLUDES += -I$(WATCOM)/h
DEFINES = -D__LIBC__
LIBOBJS=$(OBJS:.o=.obj)

include $(TOPDIR)/libc/watcom.model

CARCH =\
    -bnone                          \
    -mcmodel=$(MODEL)               \
    -march=i86                      \
    -std=c99                        \
    -fno-stack-check                \
    -Wc,-fpi87                      \
    -Wc,-zev                        \
    -Wc,-zls                        \
    -Wc,-x                          \
    -Wc,-wcd=303                    \
    -fnostdlib                      \

AARCH=\
    -m$(MODEL)                      \
    -zq                             \
    -0                              \
    -fp0                            \
    -fpi87                          \

CC=owcc
AR=wlib
AS=wasm

CFLAGS=$(CARCH) $(INCLUDES) $(DEFINES) -Wall -Os
ASFLAGS=$(AARCH)
ARFLAGS_SUB=-n -q -b -fo

%.obj: %.c
	$(CC) -c $(CFLAGS) -o $*.obj $<

%.obj: %.asm
	$(AS) $(ASFLAGS) $*.asm
	mv $*.o $*.obj
