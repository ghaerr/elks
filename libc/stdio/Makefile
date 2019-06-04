# Makefile of /libc/stdio module

include $(TOPDIR)/libc/Makefile.inc

SRCS = stdio.c printf.c scanf.c putchar.c
OBJS = $(SRCS:.c=.o)

$(OBJS): $(SRCS)

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	rm -f *.[ao]
