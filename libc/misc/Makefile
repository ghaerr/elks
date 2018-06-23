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
	popen.c \
	putenv.c \
	qsort.c \
	strtol.c \
	system.c \
	tmpnam.c \
	# end of list
	
OBJS= $(SRCS:.c=.o)

all: $(LIBC)

$(OBJS): $(SRCS)

$(LIBC): $(OBJS)
	$(AR) $(ARFLAGS) $@ $?

clean:
	rm -f *.o
