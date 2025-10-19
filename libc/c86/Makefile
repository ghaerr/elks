# Makefile of libc/c86 module

COMPILER ?= c86
LIB ?= out.lib

include $(TOPDIR)/libc/$(COMPILER).inc

OBJS = $(patsubst %.s,%.o8j,$(wildcard *.s))
OBJS += stackavail.o

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

%.o8j: %.s
	$(AS) $(ASFLAGS) -o $@ -l $*.lst $^


clean:
	$(RM) *.o8j *.lib *.lst
