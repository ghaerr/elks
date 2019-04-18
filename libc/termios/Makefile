# Makefile of /libc/termios module

include $(TOPDIR)/libc/Makefile.inc

SRCS= termios.c ttyname.c
OBJS= $(SRCS:.c=.o)

$(OBJS): $(SRCS)

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	rm -f *.[ao]
