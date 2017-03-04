# Makefile for sash

BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

OBJS = sash.o cmds.o cmd_dd.o cmd_ed.o cmd_grep.o cmd_ls.o cmd_tar.o utils.o

all: sash

sash:	$(OBJS)
	$(CC) $(LDFLAGS) -o sash $(OBJS)

clean:
	rm -f core sash $(OBJS)

max_rfs: install

rfs: install

net_rfs: rfs

min_rfs: install

smin_rfs: sash
	cp -p sash $(TARGET_MNT)/bin/sh

install: sash
	cp -p sash $(TARGET_MNT)/bin/sash

$(OBJS): sash.h config.h
