#
# Makefile for sash
#
include ../Make.defs

#OBJS = sash.o cmds.o cmd_ls.o utils.o
OBJS = sash.o cmds.o cmd_dd.o cmd_ed.o cmd_grep.o cmd_ls.o cmd_tar.o utils.o

sash:	$(OBJS)
	$(CC) $(LDFLAGS) -o sash $(OBJS)

clean:
	rm -f $(OBJS) sash

rfs: install

min_rfs: install

install: sash
	cp -p sash $(TARGET_MNT)/bin/sash

$(OBJS): sash.h config.h
