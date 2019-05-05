# Makefile for urlget

BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

SRCS = net.c urlget.c
OBJS = $(SRCS:.c=.o)

all:	urlget

urlget:	$(OBJS)
	$(LD) $(LDFLAGS) -o urlget $(OBJS) $(LDLIBS)

install: urlget
	install urlget $(DESTDIR)/bin
ifdef CONFIG_IMG_LINK
	ln -sf urlget $(DESTDIR)/bin/ftpget
else
	install urlget $(DESTDIR)/bin/ftpget
endif

clean:
	rm -f urlget *.o

