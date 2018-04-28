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

install: sash
	sudo install sash $(TARGET_MNT)/bin/sash
	sudo ln -sf sash $(TARGET_MNT)/bin/sh

$(OBJS): sash.h config.h
