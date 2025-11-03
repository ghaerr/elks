# Makefile include for OpenWatcom build

ifeq "$(TOPDIR)" ""
$(error TOPDIR environment variable not set)
endif

#ifeq "$(WATCOM)" ""
#$(error WATCOM environment variable not set)
#endif

INCLUDES = -I$(TOPDIR)/libc/include -I$(TOPDIR)/elks/include
# Watcom headers now included in libc/include/watcom
INCLUDES += -I$(TOPDIR)/libc/include/watcom
# Don't pull in headers from Watcom installation
#INCLUDES += -I$(WATCOM)/h

DEFINES = -D__LIBC__
LIBOBJS=$(OBJS:.o=.o$(MODEL)j)

ifndef MODEL
include $(TOPDIR)/libc/watcom.model
endif

CARCH =\
    -bnone                          \
    -mcmodel=$(MODEL)               \
    -march=i86                      \
    -std=c99                        \
    -fsigned-char                   \
    -msoft-float                    \
    -fno-stack-check                \
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

%.o$(MODEL)j: %.c
	$(CC) -c $(CFLAGS) -o $*.o$(MODEL)j $<

%.o$(MODEL)j: %.asm
	$(AS) $(ASFLAGS) $*.asm
	mv $*.o $*.o$(MODEL)j
