# Makefile of /libc/string module

include $(TOPDIR)/libc/Makefile.inc

OBJS = \
	bzero.o \
	memccpy.o \
	memchr.o \
	memcmp.o \
	memcpy-c.o \
	memmove.o \
	memset-c.o \
	fmemset-c.o \
	movedata.o \
	strcasecmp.o \
	strcat.o \
	strchr.o \
	strcmp-c.o \
	strcspn.o \
	strcpy-c.o \
	strdup.o \
	strlen-c.o \
	strncasecmp.o \
	strncat.o \
	strncmp.o \
	strncpy.o \
	strpbrk.o \
	strrchr.o \
	strspn.o \
	strstr.o \
	strtok.o \
	# end of list

.PHONY: all

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	rm -f $(OBJS) out.a
