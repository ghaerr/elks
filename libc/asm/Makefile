# Makefile of /libc/asm module

include $(TOPDIR)/libc/Makefile.inc

SRCS = \
    memcpy-s.s \
    memset-s.s \
    strcpy-s.s \
    strlen-s.s \
    # end of list

OBJS = $(SRCS:.s=.o)

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	rm -f *.[ao]
