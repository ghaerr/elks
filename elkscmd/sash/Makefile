#
# Makefile for sash
#
include ../Make.defs


#OBJS = sash.o cmds.o cmd_ls.o utils.o
OBJS = sash.o cmds.o cmd_dd.o cmd_ed.o cmd_grep.o cmd_ls.o cmd_tar.o utils.o

sash:	$(OBJS)
	bcc $(LDFLAGS) -o sash $(OBJS)

clean:
	rm -f $(OBJS) sash

rfs: install

min_rfs: install

install: sash
	cp -p sash $(TARGET_MNT)/bin/sh

$(OBJS): sash.h config.h
