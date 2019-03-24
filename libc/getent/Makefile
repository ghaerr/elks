# Makefile of /libc/getent module

include $(TOPDIR)/libc/Makefile.inc

SRCS= utent.c pwent.c getpwuid.c getpwnam.c __getpwent.c getgrgid.c getgrnam.c __getgrent.c putpwent.c
OBJS= $(SRCS:.c=.o)

$(OBJS): $(SRCS)

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	rm -f *.[ao]
