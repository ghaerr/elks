# Makefile for sash

BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

MIN_BIN 	= sash
MIN_LIB 	= 
MIN_SBIN	= 
MIN_USR_BIN	= 
MIN_USR_LIB	= 
MIN_USR_SBIN	= 

NET_BIN 	= 
NET_LIB 	= 
NET_SBIN	= 
NET_USR_BIN	= 
NET_USR_LIB	= 
NET_USR_SBIN	= 

STD_BIN 	= 
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= 
STD_USR_LIB	= 
STD_USR_SBIN	= 

include $(BASEDIR)/Make.rules

###############################################################################

OBJS = sash.o cmds.o cmd_dd.o cmd_ed.o cmd_grep.o cmd_ls.o cmd_tar.o utils.o

all: sash

sash:	$(OBJS)
	$(CC) $(LDFLAGS) -o sash $(OBJS)

clean:
	rm -f core sash $(OBJS)

rfs: install

net_rfs: rfs

min_rfs: install

smin_rfs: sash
	cp -p sash $(TARGET_MNT)/bin/sh

install: sash
	cp -p sash $(TARGET_MNT)/bin/sash

$(OBJS): sash.h config.h
