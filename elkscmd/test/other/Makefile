BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS = \
    test_exit \
    test_eth \
    test_pty \
    test_select \
    test_signal \
    # EOL

all: $(PRGS)

test_exit: test_exit.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_eth: test_eth.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_pty: test_pty.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_select: test_select.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_signal: test_signal.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	rm -f $(PRGS) *.o
