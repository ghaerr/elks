BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS = nano-2.0.6/src/nano
LIBS = ncurses-5.2/lib/libncurses.a

all: $(PRGS)

$(PRGS): $(LIBS)
	$(MAKE) -C nano-2.0.6
	elks-compress $(PRGS)

$(LIBS):
	$(MAKE) -C ncurses-5.2

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	$(MAKE) -C nano-2.0.6 clean
	$(MAKE) -C ncurses-5.2 clean
