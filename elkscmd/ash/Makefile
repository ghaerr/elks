# Makefile for ash.
#
# 19980209 Claudio Matsuoka <claudio@conectiva.com>
# Modified for ELKS and bcc

BASEDIR = ..

include $(BASEDIR)/Makefile-rules

###############################################################################

LOCALFLAGS = -DSHELL -I. -D_MINIX -D_POSIX_SOURCE -Dlint
LOCALFLAGS += -Wno-implicit-int

###############################################################################

SRCS=	builtins.c cd.c dirent.c error.c eval.c exec.c expand.c input.c \
	jobs.c mail.c main.c memalloc.c miscbltin.c mystring.c nodes.c \
	options.c parser.c redir.c show.c signames.c syntax.c trap.c \
	output.c var.c linenoise_elks.c autocomplete.c

OBJS=	builtins.o cd.o dirent.o error.o eval.o exec.o expand.o input.o \
	jobs.o mail.o main.o memalloc.o miscbltin.o mystring.o nodes.o \
	options.o parser.o redir.o show.o signames.o syntax.o trap.o \
	output.o var.o init.o \
	linenoise_elks.o autocomplete.o

# heap debugging using v7 debug malloc
#OBJS += dmalloc.o

# builtins must be manually added
OBJS += bltin/echo.o
OBJS += bltin/expr.o bltin/regexp.o bltin/operators.o

LDFLAGS += -maout-heap=12288 -maout-stack=2304

#
# Set READLINE in shell.h and add -ledit to LIBS if you want to use the
# editline package by Simmule Turner and Rich Salz.  (The big, bloated
# and GPL contaminated FSF readline should work too.)
#

CLEANFILES= $(OBJS) \
	builtins.c builtins.h init.c mkinit mknodes mksignames mksyntax \
	nodes.c nodes.h syntax.c syntax.h token.def \
	bltin/operators.h bltin/operators.c

all:	ash

ash: $(OBJS)
	$(LD) $(LDFLAGS) -o ash $(OBJS) $(LDLIBS)

install: ash
	$(INSTALL) ash $(DESTDIR)/bin/sh

clean:
	$(RM) ash $(CLEANFILES)

parser.o: token.def

token.def: mktokens
	sh mktokens

builtins.c builtins.h: builtins.table shell.h
	sh mkbuiltins shell.h builtins.table

init.o: mkinit $(SRCS)
	./mkinit '$(CC) -c $(CFLAGS) init.c' $(SRCS)

mkinit: mkinit.c
	gcc -std=c99 -O2 mkinit.c -o $@

nodes.c nodes.h: mknodes nodetypes nodes.c.pat
	./mknodes nodetypes nodes.c.pat

mknodes: mknodes.c
	gcc -std=c99 -O2 mknodes.c -o $@

#signames.c signames.h: mksignames
#	./mksignames

#mksignames: mksignames.c
#	gcc -O2 mksignames.c -o $@

syntax.c syntax.h: mksyntax
	./mksyntax

mksyntax: mksyntax.c parser.h
	gcc -O2 mksyntax.c -o $@

bltin/operators.h:	bltin/mkexpr bltin/binary_op bltin/unary_op
	cd bltin && sh mkexpr

bltin/echo.o:	bltin/echo.c
	cd bltin && $(CC) -I.. $(CFLAGS) -c echo.c

bltin/expr.o:	bltin/expr.c
	cd bltin && $(CC) -I.. $(CFLAGS) -c expr.c

bltin/operators.o:	bltin/operators.c
	cd bltin && $(CC) -I.. $(CFLAGS) -c operators.c

bltin/regexp.o:	bltin/regexp.c
	cd bltin && $(CC) -I.. $(CFLAGS) -c regexp.c

# Dependencies you say?  This will have to do.
$(OBJS): error.h eval.h exec.h expand.h init.h input.h \
	jobs.h machdep.h mail.h main.h memalloc.h mystring.h options.h \
	output.h parser.h redir.h shell.h trap.h var.h \
	builtins.h nodes.h signames.h syntax.h

bltin/expr.o bltin/operators.o:	bltin/operators.h
bltin/operators.c: bltin/operators.h
