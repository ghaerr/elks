# Makefile of libc/watcom/syscall module

COMPILER ?= watcom
LIB ?= out.lib

include $(TOPDIR)/libc/$(COMPILER).inc

# save/restore segment registers for all functions using sys_setseg
CFLAGS += -Wc,-r
OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean:
	$(RM) *.obj *.lib
