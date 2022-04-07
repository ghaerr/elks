BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

CFLAGS += -Wno-maybe-uninitialized

###############################################################################

PRGS = basic

all: $(PRGS)

basic: basic.o host.o
	$(LD) $(LDFLAGS) -o basic basic.o host.o $(LDLIBS)

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	rm -f $(PRGS) *.o
