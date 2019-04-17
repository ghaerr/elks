###############################################################################
#
# Include standard packaging commands.

BASEDIR 	= ../..

include $(BASEDIR)/Make.defs

###############################################################################

LOCALFLAGS=-DELKS=1

VGALIB = scr_bios.o elkplan4.o mempl4.o elksutil.o romfont.o
HERCLIB = scr_herc.o elksutil.o romfont.o

all: demo demo-herc

demo: $(VGALIB) demo.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o demo demo.o $(VGALIB) 

demo-herc: $(HERCLIB) demo-herc.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o demo-herc demo-herc.o $(HERCLIB) 

clean:
	rm -f core demo *.o
