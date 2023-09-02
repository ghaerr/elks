# Makefile of /libc/getent module

include $(TOPDIR)/libc/Makefile.inc

LIB = out.a

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

$(LIB): $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	$(RM) *.[aod]
