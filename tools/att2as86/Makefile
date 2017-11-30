.PHONY: all install

# ATT to AS86 conversion utility

CFLAGS=-O2 -ansi -Wpedantic -Wall -Wextra

$(BUILDDIR)/att2as86.o: att2as86.c
	mkdir -p $(BUILDDIR)
	$(CC) -c $(CFLAGS) -o $@ $< 

all: $(BUILDDIR)/att2as86

install: $(BUILDDIR)/att2as86
	mkdir -p $(CROSSDIR)/bin
	install $< $(CROSSDIR)/bin
