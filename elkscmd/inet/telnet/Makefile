# Makefile for telnet

BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

#LDFLAGS=-i

SRC=	ttn.c ttn_conf.c

all:	telnet

telnet:	$(SRC)
	$(CC) $(CFLAGS) -I$(INC_DIR) $(LDFLAGS) -o $@ $(SRC)

max_rfs: all

net_rfs: all

install: all
	cp -p telnet $(TARGET_MNT)/bin

clean:
	rm -f telnet

