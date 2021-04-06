#
# Makefile for exomizer
#
WFLAGS = -std=c89 -Wall -Wstrict-prototypes -D_XOPEN_SOURCE=600 -pedantic
CFLAGS = $(WFLAGS) -O3 -ffast-math -fomit-frame-pointer
LDFLAGS = -s

#CFLAGS = -g $(WFLAGS)
#LDFLAGS = -g

SHARED_OBJS = getflag.o log.o buf.o
RAW_OBJS = match.o search.o exo_raw.o optimal.o output.o buf_io.o \
           chunkpool.o radix.o exo_helper.o exodec.o progress.o exo_util.o \
           vec.o
EXO_OBJS = match.o search.o exo_main.o optimal.o output.o buf_io.o \
           chunkpool.o radix.o exo_helper.o exodec.o progress.o asm.tab.o \
           lex.yy.o parse.o expr.o pc.o vec.o named_buffer.o map.o desfx.o \
	   6502emu.o exo_util.o areatrace.o sfxdecr.o table.o perf.o
BAS_OBJS = bas_main.o bprg_renumber.o bprg_link_patch.o bprg_trampoline.o \
           bprg.o vec.o
ALL_OBJS = $(EXO_OBJS) $(RAW_OBJS) $(BAS_OBJS) $(SHARED_OBJS)

#.SILENT:

.PHONY: build clean
.INTERMEDIATE: b2buf exoraw

build: $(MAKEFILE) exomizer exobasic

exomizer: deps $(EXO_OBJS) $(SHARED_OBJS)
	@echo "Linking $@"
	@$(CC) $(LDFLAGS) -o $@ $(EXO_OBJS) $(SHARED_OBJS)

exoraw: deps $(RAW_OBJS) $(SHARED_OBJS)
	@echo "Linking $@"
	@$(CC) $(LDFLAGS) -o $@ $(RAW_OBJS) $(SHARED_OBJS)

exobasic: deps $(BAS_OBJS) $(SHARED_OBJS)
	@echo "Linking $@"
	@$(CC) $(LDFLAGS) -o $@ $(BAS_OBJS) $(SHARED_OBJS)

clean:
	@echo "Cleaning project"
	-@$(RM) $(EXO_OBJS) $(RAW_OBJS) $(BAS_OBJS) $(SHARED_OBJS)
	-@$(RM) b2buf.o b2buf b2buf.exe sfxdecr sfxdecr.c deps
	-@$(RM) exomizer exoraw exobasic exomizer.exe exoraw.exe exobasic.exe


asm.tab.h asm.tab.c: asm.y
	bison -t -d asm.y

lex.yy.c: asm.yy
	flex -B asm.yy

sfxdecr.c: sfxdecr b2buf
	@./b2buf sfxdecr >sfxdecr.c

sfxdecr: sfxdecr.s exoraw
	@echo "Compressing $<"
	@./exoraw -q sfxdecr.s -o sfxdecr

-include deps

deps: $(wildcard *.h) asm.tab.h
	@echo "Generating dependencies"
	@$(CC) -MM $(wildcard *.c) >$@

%.o:	%.c
	@echo "Compiling $<"
	@$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

%:	%.o
	@$(CC) $(LDFLAGS) $< -o $@

tabletest: table.o tabletest.o vec.o buf.o
	@$(CC) $(CFLAGS) $^ -o $@
