# Makefile of /libc/error module

include $(TOPDIR)/libc/Makefile.inc

LIB = out.a

OBJS= \
    __assert.o \
    error.o \
    perror.o \
    # end of list

all: $(LIB)

$(LIB): $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	$(RM) *.[aod]
