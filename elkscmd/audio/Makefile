BASEDIR=..

include $(BASEDIR)/Makefile-rules

###############################################################################

PRGS = audioplay audiomix audiorecv

all: $(PRGS)

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

audioplay: audioplay.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

audiomix: audiomix.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

audiorecv: audiorecv.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -maout-heap=256 -maout-stack=1024 -o $@ $^ $(LDLIBS)

clean:
	$(RM) *.o $(PRGS)
