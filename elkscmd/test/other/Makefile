BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

KERNEL_LIBS = $(TOPDIR)/elks/arch/i86/lib/lib86.a

###############################################################################

PRGS = \
    test_exit \
    test_eth \
    test_fd \
    test_float \
    test_pty \
    test_select \
    test_signal \
    test_sigfail \
    # EOL

all: $(PRGS)

test_exit: test_exit.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_eth: test_eth.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_fd: test_fd.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o $@ test_fd.o $(TINYPRINTF) $(KERNEL_LIBS) $(LDLIBS)

test_float: test_float.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_pty: test_pty.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_select: test_select.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_signal: test_signal.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_sigfail: test_sigfail.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	$(RM) $(PRGS) *.o
