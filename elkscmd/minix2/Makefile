BASEDIR=..

LOCALFLAGS=-D_POSIX_SOURCE

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

# TODO: man mt install	# Do not compile.
# compiling but unused: pwdauth
PRGS=env lp lpd remsync synctree tget

env: env.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o env env.o $(TINYPRINTF) $(LDLIBS)

pwdauth: pwdauth.o
	$(LD) $(LDFLAGS) -o pwdauth pwdauth.o $(LDLIBS)

lp: lp.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o lp lp.o $(TINYPRINTF) $(LDLIBS)

lpd: lpd.o
	$(LD) $(LDFLAGS) -o lpd lpd.o $(LDLIBS)

man: man.o
	$(LD) $(LDFLAGS) -o man man.o $(LDLIBS)

remsync: remsync.o
	$(LD) $(LDFLAGS) -o remsync remsync.o $(LDLIBS)

synctree: synctree.o
	$(LD) $(LDFLAGS) -o synctree synctree.o $(LDLIBS)

tget: tget.o
	$(LD) $(LDFLAGS) -o tget tget.o $(LDLIBS)


all: $(PRGS)

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	rm -f core *.o $(PRGS)
