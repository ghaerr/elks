# Makefile of /libc/debug module

include $(TOPDIR)/libc/Makefile.inc

LIB = out.a

OBJS = \
	instrument.o \
	readprologue.o \
	syms.o \
	stacktrace.o \
	printreg.o \
	# end of list

#OBJS += rdtsc.o

all: $(LIB)

$(LIB): $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	$(RM) *.[aod]
