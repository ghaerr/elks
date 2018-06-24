# Makefile of /libc/termios module

include $(TOPDIR)/libc/Makefile.inc

SRCS= localtime.c ctime.c tm_conv.c asctime.c asc_conv.c
OBJS= $(SRCS:.c=.o)

all: $(LIBC)

$(OBJS): $(SRCS)

$(LIBC): $(OBJS)
	$(AR) $(ARFLAGS) $@ $?

clean:
	rm -f *.o
