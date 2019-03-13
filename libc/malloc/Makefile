# Makefile of /libc/malloc module

include $(TOPDIR)/libc/Makefile.inc

SRCS = \
	heap.c \
	malloc.c \
	# end of list

OBJS = $(SRCS:.c=.o)

.PHONY: all

all: $(LIBC)

$(OBJS): $(SRCS)

$(LIBC): $(OBJS)
	$(AR) $(ARFLAGS) $@ $?

clean:
	rm -f *.o
