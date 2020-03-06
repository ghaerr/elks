# Makefile of /libc/misc module

include $(TOPDIR)/libc/Makefile.inc

CFLAGS	+= -DUSE_ATOL_AS_ATOI

LIB = out.a

OBJS = \
	aliases.o \
	atexit.o \
	atoi.o \
	atol.o \
	crypt.o \
	ctype.o \
	getcwd.o \
	getenv.o \
	getopt.o \
	itoa.o \
	ltostr.o \
	mktemp.o \
	popen.o \
	putenv.o \
	qsort.o \
	rand.o \
	strtol.o \
	system.o \
	tmpnam.o \
	ultostr.o \
	# end of list

all: $(LIB)

$(LIB): $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	$(RM) $(OBJS) $(LIB)
