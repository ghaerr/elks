include ../Make.defs

CFLAGS= $(CFLBASE)
LFLAGS= -s

xvi: xvi.o
	$(CC) $(LFLAGS) xvi.o -o xvi

rfs: xvi
	cp -p xvi $(TARGET_MNT)/bin$
	
min_rfs: xvi

clean:
	rm -f *.o xvi
