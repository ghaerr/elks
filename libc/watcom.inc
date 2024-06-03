# Makefile include for OpenWatcom build

ifeq "$(WATDIR)" ""
$(error Must set WATDIR environment variable to top of OpenWatcom C directory)
endif

INCLUDES = -I$(TOPDIR)/libc/include -I$(TOPDIR)/elks/include
INCLUDES += -I$(WATDIR)/bld/hdr/dos/h
DEFINES = -D__LIBC__
LIBOBJS=$(OBJS:.o=.obj)

ARCH =\
    -bos2                           \
    -mcmodel=c                      \
    -march=i86                      \
    -std=c99                        \
    -fno-stack-check                \
    -Wc,-fpi87                      \
    -Wc,-zev                        \
    -fnostdlib                      \
    -v                              \

CC=owcc
AR=wlib
ARFLAGS_SUB=-n -b -fo

CFLAGS=$(ARCH) $(INCLUDES) $(DEFINES) -Wall -Os

%.obj: %.c
	$(CC) $(CFLAGS) -c -o $*.obj $<
