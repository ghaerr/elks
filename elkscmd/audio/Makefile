BASEDIR=..

include $(BASEDIR)/Makefile-rules

###############################################################################

PRGS = play audiomix audiorcv

all: $(PRGS)

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

play: play.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

audiomix: audiomix.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

audiorcv: audiorcv.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -maout-heap=256 -maout-stack=1024 -o $@ $^ $(LDLIBS)

clean:
	$(RM) *.o $(PRGS)
