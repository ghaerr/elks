###############################################################################
#
# Include standard packaging commands.

BASEDIR 	= ../..

include $(BASEDIR)/Make.rules

###############################################################################

CC=bcc

LOCALFLAGS=-DELKS

VGALIB = scr_bios.o elkplan4.o mempl4.o elksutil.o romfont.o

all: demo

demo: $(VGALIB) demo.o
	$(CC) $(LDFLAGS) -o demo demo.o $(VGALIB) 

clean:
	rm -f core demo *.o
