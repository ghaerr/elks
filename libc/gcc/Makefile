# Makefile of /libc/gcc module

include $(TOPDIR)/libc/Makefile.inc

SRCS = divmodsi3.s ldivmod.s ashlsi3.s
OBJS = $(SRCS:.s=.o)

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	rm -f *.[ao]

$(OBJS): $(SRCS)
