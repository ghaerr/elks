# Makefile of libc/watcom/asm module

COMPILER ?= watcom
LIB ?= out.lib

include $(TOPDIR)/libc/$(COMPILER).inc

OBJS = $(patsubst %.asm,%.o$(MODEL)j,$(wildcard *.asm))

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean:
	$(RM) *.o$(MODEL)j *.lib
