#
# Makefile for exomizer decruncher
#
WFLAGS = -std=c89 -Wall -Wstrict-prototypes -D_XOPEN_SOURCE=600 -pedantic
CFLAGS = $(WFLAGS) -O3 -ffast-math -fomit-frame-pointer
LDFLAGS = -s

#CFLAGS = -g $(WFLAGS)
#LDFLAGS = -g

TEST_OBJS = main.o exodecrunch.o
TEST2_OBJS = main2.o exodecr-elks.o
TEST3_OBJS = elksmain.o exodecr-elks.o

#.SILENT:

.PHONY: build

build: $(MAKEFILE) test test2 test3



test: main test1.exo test2.exo test3.exo test4.exo
	./main test1.exo test1.dec
	cmp test1.bin test1.dec
	./main test2.exo test2.dec
	cmp test2.bin test2.dec
	./main test3.exo test3.dec
	cmp test3.bin test3.dec
	./main test4.exo test4.dec
	cmp test4.bin test4.dec

test2: main2 test1.oxe test2.oxe test3.oxe test4.oxe
	./main2 test1.oxe test1.dec
	cmp test1.bin test1.dec
	./main2 test2.oxe test2.dec
	cmp test2.bin test2.dec
	./main2 test3.oxe test3.dec
	cmp test3.bin test3.dec
	./main2 test4.oxe test4.dec
	cmp test4.bin test4.dec

# test elks decompress by passing original file size and safety offset as args
test3: elksmain test1.oxe test2.oxe test3.oxe test4.oxe
	./elksmain test1.oxe test1.dec 202762 2
	cmp test1.bin test1.dec
	./elksmain test2.oxe test2.dec 196608 2
	cmp test2.bin test2.dec
	./elksmain test3.oxe test3.dec 111261 2
	cmp test3.bin test3.dec
	./elksmain test4.oxe test4.dec 5632 3
	cmp test4.bin test4.dec

main: deps $(TEST_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(TEST_OBJS)

main2: deps $(TEST2_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(TEST2_OBJS)

elksmain: deps $(TEST3_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(TEST3_OBJS)

clean:
	-$(RM) $(TEST_OBJS) $(TEST2_OBJS) main main.exe main2 main2.exe \
	*.o deps *.exo *.oxe *.dec elksmain

-include deps

deps: $(wildcard *.h)
	$(CC) -MM $(wildcard *.c) >$@

%.exo: %.bin
	../src/exomizer raw -q -C $< -o $@

%.oxe: %.bin
	../src/exomizer raw -q -C -b $< -o $@
