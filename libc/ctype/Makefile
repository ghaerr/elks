# Makefile of /libc/misc module

include $(TOPDIR)/libc/Makefile.inc

LIB = out.a

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

$(LIB): $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	$(RM) *.[aod]
