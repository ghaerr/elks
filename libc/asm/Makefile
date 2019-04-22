# Makefile of /libc/asm module

include $(TOPDIR)/libc/Makefile.inc

SRCS = \
    memcpy-s.S \
    memset-s.S \
    strcpy-s.S \
    strlen-s.S \
    # end of list

OBJS = $(SRCS:.S=.o)

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	rm -f *.[ao]
