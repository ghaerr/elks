# Makefile of /libc/debug module

COMPILER ?= ia16
LIB ?= out.a

include $(TOPDIR)/libc/$(COMPILER).inc

OBJS = prectimer.o

ifeq "$(COMPILER)" "ia16"
OBJS += \
	instrument.o \
	readprologue.o \
	syms.o \
	stacktrace.o \
	printreg.o \
	prectimer.o \
	# end of list

#OBJS += rdtsc.o
endif

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean:
	$(RM) *.[aod]
