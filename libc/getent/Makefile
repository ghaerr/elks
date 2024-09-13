# Makefile of /libc/getent module

COMPILER ?= ia16
LIB ?= out.a

include $(TOPDIR)/libc/$(COMPILER).inc

OBJS = \
    utent.o \
    pwent.o \
    getpwuid.o \
    getpwnam.o \
    __getpwent.o \
    grent.o \
    getgrgid.o \
    getgrnam.o \
    __getgrent.o \
    putpwent.o \
    # end of list

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean:
	$(RM) *.[aod]
