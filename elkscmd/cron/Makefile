# Makefile for cron

BASEDIR=..

include $(BASEDIR)/Makefile-rules

###############################################################################

LOCALFLAGS = -Wno-implicit-int -Wno-return-type $(OPTIONS)

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
	$(RM) $(progs) *.o
	
install: cron
	$(INSTALL) cron $(DESTDIR)/bin/cron
	$(INSTALL) crontab $(DESTDIR)/bin/crontab

cron.o: cron.c cron.h config.h
crontab.o: crontab.c cron.h config.h
readcrontab.o: readcrontab.c cron.h config.h
lib.o: lib.c cron.h config.h
