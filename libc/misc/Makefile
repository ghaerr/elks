# Makefile of /libc/misc module

COMPILER ?= ia16
LIB ?= out.a

include $(TOPDIR)/libc/$(COMPILER).inc

OBJS = \
	aliases.o \
	atexit.o \
	atof.o \
	atoi.o \
	atol.o \
	basename.o \
	bsr.o \
	crypt.o \
	devname.o \
	dirname.o \
	dtostr.o \
	ecvt.o \
	getcwd.o \
	getenv.o \
	getpass.o \
	getopt.o \
	itoa.o \
	ltoa.o \
	ltostr.o \
	lltostr.o \
	mktemp.o \
	popen.o \
	putenv.o \
	qsort.o \
	rand.o \
	strtod.o \
	strtol.o \
	strtoul.o \
	system.o \
	tmpnam.o \
	uitoa.o \
	ultoa.o \
	ultostr.o \
	ulltostr.o \
	wildcard.o \
	# end of list

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean:
	$(RM) *.[aod]
