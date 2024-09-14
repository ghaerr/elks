# Makefile of /libc/asm module

COMPILER ?= ia16
LIB ?= out.a

include $(TOPDIR)/libc/$(COMPILER).inc

SRCS = \
    memcpy-s.S \
    memset-s.S \
    strcpy-s.S \
    strlen-s.S \
    divmod.S \
    # end of list

LEFTOUT = \
    memcmp-s.S \
    strcmp-s.S \
    # end of list

OBJS = $(SRCS:.S=.o)

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean:
	$(RM) *.[aod]
