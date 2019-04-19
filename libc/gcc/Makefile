# Makefile of /libc/gcc module

include $(TOPDIR)/libc/Makefile.inc

SRCS = divmodsi3.S ldivmod.S ashlsi3.S
OBJS = $(SRCS:.S=.o)

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	rm -f *.[ao]

$(OBJS): $(SRCS)
