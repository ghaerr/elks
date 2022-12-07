# Makefile of /libc/termcap module

include $(TOPDIR)/libc/Makefile.inc

SRCS= regex.c
OBJS= $(SRCS:.c=.o)

$(OBJS): $(SRCS)

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	$(RM) *.[aod]
