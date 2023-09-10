# Makefile of /libc/misc module

include $(TOPDIR)/libc/Makefile.inc

LIB = out.a

OBJS = \
	aliases.o \
	atexit.o \
	atof.o \
	atoi.o \
	atol.o \
	basename.o \
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
	ultostr.o \
	ulltostr.o \
	wildcard.o \
	# end of list

OBJS += instrument.o

all: $(LIB)

$(LIB): $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	$(RM) *.[aod]
