BASEDIR=..

include $(BASEDIR)/Makefile-rules

###############################################################################

###############################################################################

all: xvi

xvi: xvi.o
	$(CC) $(LDFLAGS) xvi.o -o xvi

install: xvi
	$(INSTALL) xvi $(DESTDIR)/bin

clean:
	$(RM) xvi *.o
