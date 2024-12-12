# Makefile of /libc/malloc module

COMPILER ?= ia16
LIB ?= out.a

include $(TOPDIR)/libc/$(COMPILER).inc
# allocations smaller than MCHUNK words (not bytes) are rounded up,
# larger requests are allocated from heap as is.
CFLAGS	+= -DMCHUNK=16
#CFLAGS	+= -DMINALLOC
#CFLAGS	+= -DLAZY_FREE
CFLAGS	+= -DVERBOSE=1
#CFLAGS	+= -DL_alloca

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
	dprintf.o \
	realloc.o \
	sbrk.o \
	fmemalloc.o \
	fmemfree.o \

IA16OBJS = \
	stackcheck.o \

ifeq "$(COMPILER)" "ia16"
OBJS += $(IA16OBJS)
endif

.PHONY: all

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean:
	$(RM) *.[aod]
