# Makefile of /libc/debug module

COMPILER ?= ia16
LIB ?= out.a

include $(TOPDIR)/libc/$(COMPILER).inc

OBJS = \
	instrument.o \
	readprologue.o \
	syms.o \
	stacktrace.o \
	printreg.o \
	ptostr.o \
	# end of list

#OBJS += rdtsc.o

ifeq ($(CONFIG_ARCH_IBMPC), y)
OBJS += prectimer.o
endif

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean:
	$(RM) *.[aod]
