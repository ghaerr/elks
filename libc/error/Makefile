# Makefile of /libc/misc module

include $(TOPDIR)/libc/Makefile.inc

SRCS= __assert.c error.c perror.c
OBJS= $(SRCS:.c=.o)

$(OBJS): $(SRCS)

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	$(RM) *.[aod]
