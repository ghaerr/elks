# Makefile of /libc/gcc module

include $(TOPDIR)/libc/Makefile.inc

# It is feasible to now link programs with the real libgcc, so this libgcc
# module is not really needed any more.  -- tkchia 20190603
# SRCS = divmodsi3.s ldivmod.s ashlsi3.s
SRCS =
OBJS = $(SRCS:.s=.o)

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	$(RM) *.[aod]

$(OBJS): $(SRCS)
