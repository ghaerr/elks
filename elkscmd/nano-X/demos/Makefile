CFLAGS = -g -I../server/nanox
LDFLAGS = -g -L../server -lnano-X
BINDIR = .
PROGS = info demo landmine world demo2

all:	$(PROGS)

install:	$(PROGS)
	cp $(PROGS) $(BINDIR)
	cd $(BINDIR); strip $(PROGS); chmod 755 $(PROGS)

clean:
	rm -f $(PROGS) *.o core

demo:	demo.c
	$(CC) $(CFLAGS) -o demo demo.c $(LDFLAGS)

landmine: landmine.c
	$(CC) $(CFLAGS) -o landmine landmine.c $(LDFLAGS)

world: world.c
	$(CC) $(CFLAGS) -o world world.c $(LDFLAGS)

info: info.c
	$(CC) $(CFLAGS) -o info info.c $(LDFLAGS)

demo2: demo2.c
	$(CC) $(CFLAGS) -o demo2 demo2.c $(LDFLAGS)
