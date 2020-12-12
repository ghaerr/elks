##################################################################################

# Makefile for cron

BASEDIR=..

include $(BASEDIR)/Make.defs

LOCALFLAGS = -Wno-implicit-int -Wno-return-type $(OPTIONS)

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

progs=cron crontab

common=readcrontab.o lib.o

all: $(progs)

install: $(progs)

cron: cron.o runjob.o $(common)
	$(CC) $(CFLAGS) -maout-heap=14336 -o $@ cron.o runjob.o $(common)

crontab: crontab.o $(common)
	$(CC) $(CFLAGS) -maout-heap=14336 -o $@ crontab.o $(common)

clean:
	rm -f $(progs) *.o
	
install: cron
	$(INSTALL) cron $(DESTDIR)/bin/cron
	$(INSTALL) crontab $(DESTDIR)/bin/crontab

cron.o: cron.c cron.h config.h
crontab.o: crontab.c cron.h config.h
readcrontab.o: readcrontab.c cron.h config.h
lib.o: lib.c cron.h config.h
