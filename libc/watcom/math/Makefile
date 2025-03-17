# Makefile of libc/watcom/math module

COMPILER ?= watcom
LIB ?= out.lib

include $(TOPDIR)/libc/$(COMPILER).inc

CFLAGS += -D__ELKS__ -D__GENERIC__
OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean:
	$(RM) *.ocj *.osj *.omj *.olj *.lib
