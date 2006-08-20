#
#       Makefile for MSDOS tools
#

BASEDIR = ..

include $(BASEDIR)/Make.defs

LOCALFLAGS =

###############################################################################
#
# Include standard packaging commands.

MIN_BIN 	= 
MIN_LIB 	= 
MIN_SBIN	= 
MIN_USR_BIN	= 
MIN_USR_LIB	= 
MIN_USR_MAN	= 
MIN_USR_SBIN	= 

NET_BIN 	= 
NET_LIB 	= 
NET_SBIN	= 
NET_USR_BIN	= 
NET_USR_LIB	= 
NET_USR_MAN	= 
NET_USR_SBIN	= 

STD_BIN 	= 
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= mcopy mdel mdir mkdfs mmd mrd mread mren mtype mwrite
STD_USR_LIB	= 
STD_USR_MAN	= mcopy.1 mdel.1 mdir.1 mkdfs.1 mmd.1 mrd.1 mread.1 mren.1 \
		  mtype.1 mwrite.1
STD_USR_SBIN	= 

include $(BASEDIR)/Make.rules

###############################################################################

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
mren:	mren.o fixname.o putfat.c isdir.o $(CMNOBJ)
mcopy:	mcopy.o

install: all
	cp $(PROGS) $(TARGET_MNT)/$(BINDIR)/

install-man:
	@for i in $(PROGS); do \
		M=`echo $$i | sed 's/\(.\)/\u\1/'`.1; \
		echo Installing $$M; \
		compress < $$M > $(TARGET_MNT)/$(MANDIR)/$$M.Z; \
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
