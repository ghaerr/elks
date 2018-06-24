# Makefile of /libc/termcap module

include $(TOPDIR)/libc/Makefile.inc

SRCS= regex.c
OBJS= $(SRCS:.c=.o)

all: $(LIBC)

$(OBJS): $(SRCS)

$(LIBC): $(OBJS)
	$(AR) $(ARFLAGS) $@ $?

clean:
	rm -f *.o
