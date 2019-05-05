BASEDIR=../../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=echoserver echoclient

all: $(PRGS)

echoserver: echoserver.o
	$(LD) $(LDFLAGS) -o echoserver echoserver.o $(LDLIBS)

echoclient: echoclient.o
	$(LD) $(LDFLAGS) -o echoclient echoclient.o $(LDLIBS)

install: $(PRGS)
	sudo install $(PRGS) $(DESTDIR)/bin

clean:
	rm -f *.o $(PRGS)
