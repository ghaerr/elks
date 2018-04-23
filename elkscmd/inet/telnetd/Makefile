# Makefile for telnet

BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

SRC=	telnetd.c 

all:	telnetd

telnetd: $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $(SRC)

install: telnetd
	sudo install telnetd $(TARGET_MNT)/bin

clean:
	rm -f *.o telnetd

