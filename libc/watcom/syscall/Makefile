# Makefile of libc/watcom/syscall module

COMPILER ?= watcom
LIB ?= out.lib

include $(TOPDIR)/libc/$(COMPILER).inc

OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean:
	$(RM) *.obj *.lib
