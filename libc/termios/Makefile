# Makefile of /libc/termios module

COMPILER ?= ia16
LIB ?= out.a

include $(TOPDIR)/libc/$(COMPILER).inc

# these are unimplemented
#CFLAGS += -DL_tcdrain -DL_tcflow -DL_tcgetpgrp -DL_cfmakeraw

# TCSBRK is unimplemented except for with elksemu
CFLAGS += -DL_tcsendbreak

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
