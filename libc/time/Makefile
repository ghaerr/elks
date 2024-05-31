# Makefile of /libc/time module

COMPILER ?= ia16
LIB ?= out.a

include $(TOPDIR)/libc/$(COMPILER).inc

OBJS = \
    gmtime.o \
    localtime.o \
    ctime.o \
    tm_conv.o \
    asctime.o \
    asc_conv.o \
    tzset.o \
    mktime.o \
    strftime.o \

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean:
	$(RM) *.[aod]
