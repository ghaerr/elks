# Makefile of /libc/termios module

include $(TOPDIR)/libc/Makefile.inc

SRCS= gmtime.c localtime.c ctime.c tm_conv.c asctime.c asc_conv.c
OBJS= $(SRCS:.c=.o)

$(OBJS): $(SRCS)

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	rm -f *.[ao]
