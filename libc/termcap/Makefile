# Makefile of /libc/termcap module

include $(TOPDIR)/libc/Makefile.inc

SRCS= termcap.c tparam.c
OBJS= $(SRCS:.c=.o)

$(OBJS): $(SRCS)

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	rm -f *.[ao]
