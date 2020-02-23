# Makefile of /libc/malloc module

include $(TOPDIR)/libc/Makefile.inc

#CFLAGS	+= -DL_alloca
#CFLAGS	+= -DLAZY_FREE
CFLAGS	+= -DMCHUNK=2048
#CFLAGS	+= -DMINALLOC
#CFLAGS	+= -DVERBOSE

OBJS = \
	__alloca_alloc.o \
	__freed_list.o \
	__mini_malloc.o \
	alloca.o \
	brk.o \
	calloc.o \
	free.o \
	malloc.o \
	noise.o \
	realloc.o \
	sbrk.o \

.PHONY: all

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	$(RM) $(OBJS)
