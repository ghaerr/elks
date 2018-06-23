# Makefile of /libc/stdio module

include $(TOPDIR)/libc/Makefile.inc

SRCS = stdio.c printf.c scanf.c
OBJS = $(SRCS:.c=.o)

all: $(LIBC)

$(LIBC): $(OBJS)
	$(AR) $(ARFLAGS) $@ $?

$(OBJS): $(SRCS)

clean:
	rm -f *.o
