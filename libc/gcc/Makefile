# Makefile of /libc/gcc module

include $(TOPDIR)/libc/Makefile.inc

SRCS = divmodsi3.s ldivmod.s ashlsi3.s
OBJS = $(SRCS:.s=.o)

all: $(LIBC) 

$(LIBC): $(OBJS)
	$(AR) $(ARFLAGS) $@ $?

$(OBJS): $(SRCS)

clean:
	rm -f *.o
