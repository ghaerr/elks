# Makefile of libc/watcom/asm module

COMPILER ?= watcom
LIB ?= out.lib

include $(TOPDIR)/libc/$(COMPILER).inc

OBJS = $(patsubst %.asm,%.obj,$(wildcard *.asm))

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean:
	$(RM) *.obj *.lib
