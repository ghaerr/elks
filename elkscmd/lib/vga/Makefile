###############################################################################
#
# Include standard packaging commands.

BASEDIR 	= ../..

MIN_BIN 	= 
MIN_LIB 	= 
MIN_SBIN	= 
MIN_USR_BIN	= 
MIN_USR_LIB	= 
MIN_USR_SBIN	= 

NET_BIN 	= 
NET_LIB 	= 
NET_SBIN	= 
NET_USR_BIN	= 
NET_USR_LIB	= 
NET_USR_SBIN	= 

STD_BIN 	= 
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= demo
STD_USR_LIB	= 
STD_USR_SBIN	= 

include $(BASEDIR)/Make.rules

###############################################################################

CC=bcc

CFLAGS=-ansi -DELKS

VGALIB = scr_bios.o elkplan4.o mempl4.o elksutil.o romfont.o

all: demo

demo: $(VGALIB) demo.o
	$(CC) $(LDFLAGS) -o demo demo.o $(VGALIB) 

clean:
	rm -f core demo *.o
