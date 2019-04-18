# Makefile of /libc/malloc module

include $(TOPDIR)/libc/Makefile.inc

SRCS = \
	heap.c \
	malloc.c \
	# end of list

OBJS = $(SRCS:.c=.o)

.PHONY: all

$(OBJS): $(SRCS)

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	rm -f *.[ao]
