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

max_rfs: all

net_rfs: all

install: all
	cp -p urlget $(TARGET_MNT)/bin
#urlget.c checks its own name to branch to ftp	
	ln -s urlget $(TARGET_MNT)/bin/ftpget

clean:
	rm -f urlget *.o

