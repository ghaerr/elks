# Makefile for telnet

BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

SRCS = ttn.c ttn_conf.c
OBJS = $(SRCS:.c=.o)

all:	telnet

telnet:	$(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

install: telnet
	sudo install telnet $(TARGET_MNT)/bin

clean:
	rm -f $(OBJS) telnet
