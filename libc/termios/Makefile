# Makefile of /libc/termios module

include $(TOPDIR)/libc/Makefile.inc

SRCS= termios.c ttyname.c
OBJS= $(SRCS:.c=.o)

all: $(LIBC)

$(OBJS): $(SRCS)

$(LIBC): $(OBJS)
	$(AR) $(ARFLAGS) $@ $?

clean:
	rm -f *.o
