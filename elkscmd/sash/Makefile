# Makefile for sash

BASEDIR=..

include $(BASEDIR)/Make.defs

LOCALFLAGS = -Wno-implicit-int

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

OBJS = sash.o cmds.o cmd_dd.o cmd_ed.o cmd_grep.o cmd_ls.o cmd_tar.o utils.o cmd_history.o
LDFLAGS += -maout-heap=6144 -maout-stack=3072

all: sash

sash:	$(OBJS)
	$(LD) $(LDFLAGS) -o sash $(OBJS) $(LDLIBS)

clean:
	rm -f core sash $(OBJS)

install: sash
ifdef CONFIG_APP_ASH
	$(INSTALL) sash $(DESTDIR)/bin/sash
else
	# install as /bin/sh if no ash shell
	$(INSTALL) sash $(DESTDIR)/bin/sh
endif

$(OBJS): sash.h config.h
