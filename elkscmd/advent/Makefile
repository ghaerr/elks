BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

CFLAGS += -DELKS=1
#CFLAGS += -Wno-parentheses

###############################################################################

PRGS = advent
SRCS = advent.c adventdb.c database.c english.c itverb.c lib.c saveadv.c \
      turn.c verb.c global.c

OBJS = $(SRCS:.c=.o)

HOSTPRGS = hostadvent hostadvgen
#HOSTPRGS += advent.db

all: $(PRGS)

host: $(HOSTPRGS)

advent: $(OBJS) advent.h
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

hostadvent: $(SRCS)
	$(HOSTCC) -DADVDB_PATH=\"./advent.db\" -o $@ $^

hostadvgen: advgen.c
	$(HOSTCC) -o $@ $^

advent.db: hostadvgen
	./hostadvgen > advent.db

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin
	$(INSTALL) advent.db $(DESTDIR)/lib

clean:
	$(RM) $(PRGS) $(HOSTPRGS) *.o *.map
