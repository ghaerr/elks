BASEDIR=../..

include $(BASEDIR)/Makefile-rules

###############################################################################

###############################################################################

PRGS=echoserver echoclient

all: $(PRGS)

echoserver: echoserver.o
	$(LD) $(LDFLAGS) -o echoserver echoserver.o $(LDLIBS)

echoclient: echoclient.o
	$(LD) $(LDFLAGS) -o echoclient echoclient.o $(LDLIBS)

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	$(RM) *.o $(PRGS)
