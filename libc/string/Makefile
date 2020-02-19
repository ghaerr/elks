# Makefile of /libc/string module

include $(TOPDIR)/libc/Makefile.inc

SRCS = \
	bzero.c \
	memccpy.c \
	memchr.c \
	memcmp.c \
	memcpy-c.c \
	memmove.c \
	memset-c.c \
	movedata.c \
	strcasecmp.c \
	strcat.c \
	strchr.c \
	strcmp.c \
	strcspn.c \
	strcpy-c.c \
	strdup.c \
	strlen-c.c \
	strncasecmp.c \
	strncat.c \
	strncmp.c \
	strncpy.c \
	strpbrk.c \
	strrchr.c \
	strspn.c \
	strstr.c \
	strtok.c \
	# end of list

OBJS = $(SRCS:.c=.o)

.PHONY: all

$(OBJS): $(SRCS)

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	rm -f *.[ao]
