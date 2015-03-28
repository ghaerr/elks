
CC = bcc
CFLAGS = -ansi -O -0 -I../elks/include

all: busyelks1 busyelks2 busyelks3

busyelks1: $(OBJS1) $(LIBOBJS) $(L1OBJS) busyelks1.o
	$(CC) $(CFLAGS) -s -o busyelks1 $(OBJS1) $(LIBOBJS) $(L1OBJS) busyelks1.o

busyelks2: $(OBJS2) $(LIBOBJS) busyelks2.o
	$(CC) $(CFLAGS) -s -o busyelks2 $(OBJS2) $(LIBOBJS) busyelks2.o

busyelks3: $(OBJS3) $(LIBOBJS) busyelks3.o
	$(CC) $(CFLAGS) -s -o busyelks3 $(OBJS3) $(LIBOBJS) busyelks3.o

busyelks: busyelks1 busyelks2 busyelks3

$(OBJS1): $(CFILES1) busyelks1.c

$(OBJS2): $(CFILES2) busyelks2.c

$(OBJS3): $(CFILES3) busyelks3.c

$(L1OBJS): $(L1CFILES)

$(LIBOBJS): $(LIBCFILES)

clean:
	rm -rf a.out core *~ *.bak *.o */*.o busyelks1 busyelks2 busyelks3 _install

stats:
	objdump86 */*.o | grep -A 1 text | grep '\/.*\.o' | sort -g -k 1
	objdump86 busyelks? | grep -v '\:' | grep -A 1 text
