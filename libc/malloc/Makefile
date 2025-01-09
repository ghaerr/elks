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
DEBUG_MALLOC_OBJS = dmalloc.o

# far malloc (single arena)
FAR_MALLOC_OBJS = fmalloc.o

# these objects work with any malloc
OBJS = \
	calloc.o \
	brk.o \
	sbrk.o \
	fmemalloc.o \
	dprintf.o \

# default and debug mallocs for all compilers
OBJS += $(DEFAULT_MALLOC_OBJS) $(DEBUG_MALLOC_OBJS)

# far malloc works with OWC only for now
ifeq "$(COMPILER)" "watcom"
OBJS += $(FAR_MALLOC_OBJS)
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
