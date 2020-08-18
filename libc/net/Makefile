# Makefile of /libc/net module

include $(TOPDIR)/libc/Makefile.inc

SRCS= in_aton.c in_ntoa.c in_gethostbyname.c
OBJS= $(SRCS:.c=.o)

$(OBJS): $(SRCS)

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	rm -f *.[ao]
