# Makefile of /libc/malloc module

COMPILER ?= ia16
LIB ?= out.a

include $(TOPDIR)/libc/$(COMPILER).inc

# options for default malloc (dev86) only:
# allocations smaller than MCHUNK words (not bytes) are rounded up,
# larger requests are allocated from heap as is.
DEFINES	+= -DMCHUNK=16
#DEFINES += -DVERBOSE=1
#DEFINES += -DMINALLOC
#DEFINES += -DLAZY_FREE
#DEFINES += -DL_alloca

# default malloc (dev86)
DEFAULT_MALLOC_OBJS = \
	malloc.o \
	realloc.o \
	free.o \
	__mini_malloc.o \
	__alloca_alloc.o \
	__freed_list.o \
	noise.o \
	alloca.o \

# debug malloc (v7)
DEBUG_MALLOC_OBJS = v7malloc.o

# arena malloc
ARENA_MALLOC_OBJS = amalloc.o

# these objects work with any malloc
OBJS = \
	calloc.o \
	brk.o \
	sbrk.o \
	fmemalloc.o \
	dprintf.o \

# debug malloc for all compilers
OBJS += $(DEFAULT_MALLOC_OBJS)

# arena malloc works with OWC only for now
ifeq "$(COMPILER)" "watcom"
OBJS += $(ARENA_MALLOC_OBJS)
endif

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
