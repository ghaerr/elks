# Makefile of /libc/misc module

include $(TOPDIR)/libc/Makefile.inc

SRCS= \
	aliases.c \
	atexit.c \
	atoi.c \
	atol.c \
	crypt.c \
	ctype.c \
	getcwd.c \
	getenv.c \
	getopt.c \
	itoa.c \
	ltostr.c \
	mktemp.c \
	popen.c \
	putenv.c \
	qsort.c \
	rand.c \
	strtol.c \
	system.c \
	tmpnam.c \
	# end of list
	
OBJS= $(SRCS:.c=.o)

$(OBJS): $(SRCS)

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	rm -f *.[ao]
