# Makefile for telnet

BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

SRC=	ttn.c ttn_conf.c

all:	telnet

telnet:	$(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $(SRC)

install: telnet
	sudo install telnet $(TARGET_MNT)/bin

clean:
	rm -f telnet

