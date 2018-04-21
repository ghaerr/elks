#
#       Makefile for MSDOS tools
#

BASEDIR = ..

include $(BASEDIR)/Make.defs

LOCALFLAGS =

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

COMPRESS=../misc_utils/compress.host

PREFIX	= /usr
BINDIR	= $(PREFIX)/bin
MANDIR  = $(PREFIX)/man/man1
PROGS	= mdir mread mwrite mdel mtype mmd mrd mren mcopy mkdfs
CMNOBJ	= init.o search.o unixname.o subdir.o getfat.o devices.o

all:	$(PROGS)

mkdfs:	mkdfs.o
mdir:	mdir.o match.o convdate.o isdir.o $(CMNOBJ)
mread:	mread.o match.o convdate.o $(CMNOBJ)
mwrite:	mwrite.o fixname.o putfat.o mkentry.o isdir.o $(CMNOBJ)
mdel:	mdel.o match.o putfat.o $(CMNOBJ)
mtype:	mtype.o match.o $(CMNOBJ)
mmd:	mmd.o fixname.o putfat.o mkentry.o $(CMNOBJ)
mrd:	mrd.o putfat.o $(CMNOBJ)
mren:	mren.o fixname.o putfat.o isdir.o $(CMNOBJ)
mcopy:	mcopy.o

install: $(PROGS)
	sudo install $(PROGS) $(TARGET_MNT)/$(BINDIR)/

install-man:
	@for i in $(PROGS); do \
		M=`echo $$i | sed 's/\(.\)/\u\1/'`.1; \
		echo Installing $$M; \
		$(COMPRESS) -b12 < $$M > $(TARGET_MNT)/$(MANDIR)/$$M.Z; \
	done

clean:
	rm -f $(PROGS) *.o

lint:
	$(LINT) mdir.c getfat.c init.c search.c match.c convdate.c subdir.c \
	unixname.c isdir.c
	$(LINT) mread.c getfat.c init.c search.c match.c unixname.c subdir.c
	$(LINT) mwrite.c getfat.c init.c search.c fixname.c unixname.c \
	putfat.c subdir.c mkentry.c isdir.c
	$(LINT) mdel.c getfat.c init.c search.c match.c unixname.c putfat.c \
	subdir.c
	$(LINT) mtype.c getfat.c init.c search.c match.c unixname.c subdir.c
	$(LINT) mmd.c getfat.c init.c search.c fixname.c unixname.c putfat.c \
	subdir.c mkentry.c
	$(LINT) mrd.c getfat.c init.c search.c unixname.c putfat.c subdir.c
	$(LINT) mren.c getfat.c init.c search.c fixname.c unixname.c putfat.c \
	subdir.c
	$(LINT) mcopy.c

init.o:	msdos.h devices.h
