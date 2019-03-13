# Makefile of /libc/string module

include $(TOPDIR)/libc/Makefile.inc

SRCS = \
	bzero.c \
	memcpy-c.c \
	memset-c.c \
	strcasecmp.c \
	strcspn.c \
	string.c \
	strcpy-c.c \
	strlen-c.c \
	strncasecmp.c \
	strpbrk.c \
	strspn.c \
	strstr.c \
	strtok.c \
	# end of list

OBJS = $(SRCS:.c=.o)

.PHONY: all

all: $(LIBC)

$(OBJS): $(SRCS)

$(LIBC): $(OBJS)
	$(AR) $(ARFLAGS) $@ $?

clean:
	rm -f *.o
