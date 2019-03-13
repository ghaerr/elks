# Makefile of /libc/asm module

include $(TOPDIR)/libc/Makefile.inc

SRCS = \
    memcpy-s.s \
    memset-s.s \
    strcpy-s.s \
    strlen-s.s \
    # end of list

OBJS = $(SRCS:.s=.o)

all: $(LIBC)

$(LIBC): $(OBJS)
	$(AR) $(ARFLAGS) $@ $?

clean:
	rm -f *.o
