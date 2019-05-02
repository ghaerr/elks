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
	$(SUDO) install urlget $(TARGET_MNT)/bin
ifdef CONFIG_IMG_LINK
	$(SUDO) ln -sf urlget $(TARGET_MNT)/bin/ftpget
else
	$(SUDO) install urlget $(TARGET_MNT)/bin/ftpget
endif

clean:
	rm -f urlget *.o

