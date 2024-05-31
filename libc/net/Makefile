# Makefile of /libc/net module

COMPILER ?= ia16
LIB ?= out.a

include $(TOPDIR)/libc/$(COMPILER).inc

OBJS= in_aton.o in_ntoa.o in_gethostbyname.o getsocknam.o in_connect.o in_resolv.o

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean:
	$(RM) *.[aod]
