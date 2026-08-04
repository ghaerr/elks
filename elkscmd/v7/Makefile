BASEDIR=..

include $(BASEDIR)/Makefile-rules

###############################################################################

PRGS = v7info v7mode v7show

all: $(PRGS)

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

v7info: v7info.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

v7mode: v7mode.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

v7show: v7show.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	$(RM) *.o $(PRGS)
