#
#       Makefile for MSDOS tools
#
CC = bcc

CFLAGS	= -g
LDFLAGS	= 
LINT	= lint -p
#
# Rich Salz's excelent cshar package.
# Available from comp.sources.unix archives.
#
SHAR	= makekit -m
BINDIR	= /u/viktor/bin/sun4
MANDIR  = /u/viktor/man/man1
PROGS	= mdir mread mwrite mdel mtype mmd mrd mren mcopy mkdfs
CMNOBJ = init.o search.o unixname.o subdir.o getfat.o devices.o

MKDFS   = mkdfs.o
MDIR	= mdir.o match.o convdate.o isdir.o $(CMNOBJ)
MREAD	= mread.o match.o convdate.o $(CMNOBJ)
MWRITE	= mwrite.o fixname.o putfat.o mkentry.o isdir.o $(CMNOBJ)
MDEL	= mdel.o match.o putfat.o $(CMNOBJ)
MTYPE	= mtype.o match.o $(CMNOBJ)
MMD	= mmd.o fixname.o putfat.o mkentry.o $(CMNOBJ)
MRD	= mrd.o putfat.o $(CMNOBJ)
MREN	= mren.o fixname.o putfat.c isdir.o $(CMNOBJ)
MCOPY	= mcopy.o

all:	$(PROGS)

mkdfs:  $(MKDFS)
	$(CC) $(LDFLAGS) $(MKDFS) -o mkdfs

mdir:	$(MDIR)
	$(CC) $(LDFLAGS) $(MDIR) -o mdir

mread:	$(MREAD)
	$(CC) $(LDFLAGS) $(MREAD) -o mread

mwrite:	$(MWRITE)
	$(CC) $(LDFLAGS) $(MWRITE) -o mwrite

mdel:	$(MDEL)
	$(CC) $(LDFLAGS) $(MDEL) -o mdel

mtype:	$(MTYPE)
	$(CC) $(LDFLAGS) $(MTYPE) -o mtype

mmd:	$(MMD)
	$(CC) $(LDFLAGS) $(MMD) -o mmd

mrd:	$(MRD)
	$(CC) $(LDFLAGS) $(MRD) -o mrd

mren:	$(MREN)
	$(CC) $(LDFLAGS) $(MREN) -o mren

mcopy:	$(MCOPY)
	$(CC) $(LDFLAGS) $(MCOPY) -o mcopy

install: all
	cp mdir $(BINDIR)/mdir
	cp mread $(BINDIR)/mread
	cp mwrite $(BINDIR)/mwrite
	cp mdel $(BINDIR)/mdel
	cp mtype $(BINDIR)/mtype
	cp mmd $(BINDIR)/mmd
	cp mrd $(BINDIR)/mrd
	cp mren $(BINDIR)/mren
	cp mcopy $(BINDIR)/mcopy

install-man:
	cp Mread.1 $(MANDIR)/mread.1
	cp Mdir.1 $(MANDIR)/mdir.1
	cp Mwrite.1 $(MANDIR)/mwrite.1
	cp Mdel.1 $(MANDIR)/mdel.1
	cp Mtype.1 $(MANDIR)/mtype.1
	cp Mmd.1 $(MANDIR)/mmd.1
	cp Mrd.1 $(MANDIR)/mrd.1
	cp Mren.1 $(MANDIR)/mren.1
	cp Mcopy.1 $(MANDIR)/mcopy.1

clean:
	rm $(PROGS) *.o

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

shar:
	$(SHAR) MANIFEST Customize Makefile Readme *.[1ch]

init.o:	msdos.h devices.h
