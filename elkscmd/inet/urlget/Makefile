# Makefile for urlget

BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

SRC=	net.c urlget.c

all:	urlget

urlget:	$(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) -o urlget $(SRC)

install: urlget
	sudo install urlget $(TARGET_MNT)/bin
ifdef CONFIG_IMG_LINK
	sudo ln -s urlget $(TARGET_MNT)/bin/ftpget
else
	sudo install urlget $(TARGET_MNT)/bin/ftpget
endif

clean:
	rm -f urlget *.o

