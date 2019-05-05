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
	$(LD) $(LDFLAGS) -o sash $(OBJS) $(LDLIBS)

clean:
	rm -f core sash $(OBJS)

install: sash
ifdef CONFIG_IMG_LINK
	install sash $(TARGET_MNT)/bin/sash
	ln -sf sash $(TARGET_MNT)/bin/sh
else
	install sash $(TARGET_MNT)/bin/sh
endif

$(OBJS): sash.h config.h
