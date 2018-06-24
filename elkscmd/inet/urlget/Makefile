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
	sudo install urlget $(TARGET_MNT)/bin
ifdef CONFIG_IMG_LINK
	sudo ln -s urlget $(TARGET_MNT)/bin/ftpget
else
	sudo install urlget $(TARGET_MNT)/bin/ftpget
endif

clean:
	rm -f urlget *.o

