# Makefile of /libc/termios module


COMPILER ?= ia16
LIB ?= out.a

include $(TOPDIR)/libc/$(COMPILER).inc
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

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean::
	$(RM) *.[aod]
