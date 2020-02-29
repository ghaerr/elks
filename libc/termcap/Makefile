# Makefile of /libc/termcap module

include $(TOPDIR)/libc/Makefile.inc

LIB	= out.a
OBJS	= termcap.o tparam.o

all: $(LIB)

$(LIB): $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

distclean: clean
	$(RM) $(LIB)

clean::
	$(RM) $(OBJS)
