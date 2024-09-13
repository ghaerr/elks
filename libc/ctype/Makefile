# Makefile of /libc/ctype module

COMPILER ?= ia16
LIB ?= out.a

include $(TOPDIR)/libc/$(COMPILER).inc

OBJS = \
	ctype.o \
	isalnum.o \
	isalpha.o \
	isascii.o \
	isblank.o \
	isdigit.o \
	islower.o \
	isprint.o \
	ispunct.o \
	issspace.o \
	isupper.o \
	isxdigit.o \
	toupper.o \
	tolower.o \
	# end of list

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean:
	$(RM) *.[aod]
