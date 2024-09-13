# Makefile of /libc/malloc module

COMPILER ?= ia16
LIB ?= out.a

include $(TOPDIR)/libc/$(COMPILER).inc
#CFLAGS	+= -DL_alloca
#CFLAGS	+= -DLAZY_FREE
# MCHUNK is word not byte min allocation size
CFLAGS	+= -DMCHUNK=512
#CFLAGS	+= -DMINALLOC
#CFLAGS	+= -DVERBOSE

# use V7 malloc for heap integrity checking
#OBJS = v7malloc.o calloc.o sbrk.o brk.o

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
	fmemalloc.o \

.PHONY: all

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean:
	$(RM) *.[aod]
