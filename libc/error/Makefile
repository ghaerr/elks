# Makefile of /libc/misc module

include $(TOPDIR)/libc/Makefile.inc

SRCS= __assert.c error.c perror.c
OBJS= $(SRCS:.c=.o)

all: $(LIBC)

$(OBJS): $(SRCS)

$(LIBC): $(OBJS)
	$(AR) $(ARFLAGS) $@ $?

clean:
	rm -f $(OBJS)
