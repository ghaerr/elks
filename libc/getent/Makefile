# Makefile of /libc/termios module

include $(TOPDIR)/libc/Makefile.inc

SRCS= utent.c pwent.c getpwuid.c getpwnam.c __getpwent.c getgrgid.c getgrnam.c __getgrent.c
OBJS= $(SRCS:.c=.o)

all: $(LIBC)

$(OBJS): $(SRCS)

$(LIBC): $(OBJS)
	$(AR) $(ARFLAGS) $@ $?

clean:
	rm -f *.o
