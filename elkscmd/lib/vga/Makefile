CC=bcc
CFLAGS=-ansi -DELKS

VGALIB = scr_bios.o elkplan4.o mempl4.o elksutil.o romfont.o

all: demo

demo: $(VGALIB) demo.o
	$(CC) $(LDFLAGS) -o demo demo.o $(VGALIB) 

clean:
	rm -f *.o demo
