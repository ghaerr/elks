# Didn't check this file for CC and CFLAGS

run: all

all: objchop modlink.o serial.c vals
	bcc -0 -I../include -D__KERNEL__ -c -o serial.o serial.c
	ld86 -i -0 -s -o serial.ln modlink.o serial.o
	./objchop serial.ln serial.T serial.D serial.S
	./vals | sh

modlink.o: ../System.map symchop.awk
	awk -f symchop.awk < ../System.map > modlink.s
	bcc -c modlink.s

objchop: objchop.c
	cc -o objchop objchop.c

vals: vals.c
	gcc -o vals vals.c

clean:
	rm -f modrun modrun.map
	rm -f modlink.s modlink.o
	rm -f objchop serial.ln serial.o
	rm -f serial.[STD]
	rm -f symchop.pos vals

