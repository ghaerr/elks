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

max_rfs: all

net_rfs: all

install: all
	cp -p telnetd $(TARGET_MNT)/bin

clean:
	rm -f *.o telnetd

