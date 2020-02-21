# Makefile of /libc/termios module

include $(TOPDIR)/libc/Makefile.inc

OBJS = \
	cfgetispeed.o \
	cfgetospeed.o \
	cfmakeraw.o \
	cfsetispeed.o \
	cfsetospeed.o \
	isatty.o \
	tcdrain.o \
	tcflow.o \
	tcflush.o \
	tcgetattr.o \
	tcgetpgrp.o \
	tcsendbreak.o \
	tcsetattr.o \
	tcsetpgrp.o \
	ttyname.o \

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean::
	$(RM) $(OBJS) out.a
