include ../../Make.defs

PRGS=netstat

SPRGS=

CFLAGS=-0 -O -ansi -I$(ELKSNET_DIR)

all: $(PRGS)

rfs: install

min_rfs: install

smin_rfs: install_sibo

install_sibo: all
	cp -p $(SPRGS) $(TARGET_MNT)/bin

install: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

#fsck: fsck.c
#	$(CC) $(CFLAGS) $(LDFLAGS) -ansi fsck.c -o fsck -Hffff

netstat: netstat.o
	$(CC) $(LDFLAGS) netstat.o -o netstat

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(FORMATMOD) $(PRGS)
