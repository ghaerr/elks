.PHONY: all install

# ATT to AS86 conversion utility

CFLAGS=-O2 -ansi -Wpedantic -Wall -Wextra

$(BUILDDIR)/att2as86.o: $(SRCDIR)/att2as86/att2as86.c
	$(CC) -c $(CFLAGS) -o $@ $< 

all: $(BUILDDIR)/att2as86

install: $(BUILDDIR)/att2as86
	install $< $(PREFIX)/bin
