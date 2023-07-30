# combined Makefile for ELVIS - a clone of `vi`
#
###############################################################################

BASEDIR = ..

include $(BASEDIR)/Make.defs

LOCALFLAGS=-DM_SYSV -DCRUNCH -DNO_MKEXRC -DNO_CURSORSHAPE -DNO_CHARATTR \
	-DNO_SHOWMODE -DNO_MODELINE -DNO_OPTCOLS -DNO_DIGRAPH -DNO_ABBR \
	-DNO_AT -DNO_SENTENCE -DNO_ERRLIST -DNO_EXTENSIONS -DNO_RECYCLE \
	-DNO_MAGIC -DNO_CHARSEARCH -DNO_NONEX -DDATE=\"`date +%D`\"

###############################################################################
#
# After editing this Makefile as described below, you should...
#
# Use `make` to compile all programs
# Use `make install` to copy the programs to the BIN directory
# Use `make clean` to remove all object files
# Use `make clobber` to remove everything except source & documentation
# Use `make tags` to build new "tags" and "refs" files
# Use `make uue` to produce uuencoded compressed tar archives of the source
# Use `make sh` to produce shar archives of the source
# Use `make print` to print the Elvis documentation
#
# Several groups of Makefile settings are included below.  Choose *ONE* group
# of settings for your particular system, and leave the others commented out.
# The meanings of these settings are:
#	O	the filename extension for unlinked object files -- usually .o
#	E	the filename extension for executable files -- usually null
#	EXTRA	version-specific object files used in elvis
#	EXTRA2	version-specific object files used in elvis, virec, & refont
#	LIBS	any special libraries, such as "-ltermcap"
#	BIN	directory where executables should be installed
#	CC	the C compiler command, possibly with "memory model" flags
#	CFLAGS	compiler flags used to select compile-time options
#	OF	link flag to control the output file's name -- usually -o<space>
#	RF	flag used to denote "compile but don't link" -- usually -c
#	DATE	a "cc" flag that defines "DATE".  e.g. DATE=-DDATE=\"7/4/76\"
#	EVAL	the word "eval", if DATE requires it
#	PROGS	the list of all programs
#	CHMEM	any extra commands to be run after ELVIS is linked
#	SORT	if the "tags" file must be sorted, then SORT=-DSORT
#	INST	installation method: inst.dos, inst.tos, inst.os9, or inst.unix
#	RM	the name of a program that deletes files
#	PR1	used to print documentation -- typically "refont -c"
#	PR2	used to send text to printer -- typically "| lpr"
#	DUMMY	usually nothing, but OS9 needs "dummy"
#	DOC	name of "doc" directory, with a trailing slash

#---- These settings are recommended for System-V UNIX and SCO XENIX-386 ----

O=	.o
EXTRA=
EXTRA2=
LIBS=
BIN=	/bin

OF=	-o 
RF=	-c
DATE=	-DDATE=\'\"`date`\"\'
EVAL=	eval 
PROGS=	elvis ctags ref virec refont
CHMEM=	
SORT=	-DSORT
INST=	inst.unix
RM=	rm -f
PR1=	refont -c
PR2=	| lp
DUMMY=
DOC=	doc/

#---- These settings are recommended for SCO XENIX-286 ----
#O=	.o
#E=
#EXTRA=
#EXTRA2=
#LIBS=	-ltermcap
#BIN=	/usr/local/bin
#CC=	cc -M2s -i
#CFLAGS=	-DM_SYSV -Ox -DCS_IBMPC
#OF=	-o 
#RF=	-c
#DATE=	-DDATE=\'\"`date`\"\'
#EVAL=	eval 
#PROGS=	elvis ctags ref virec refont
#CHMEM=	
#SORT=	-DSORT
#INST=	inst.unix
#RM=	rm -f
#PR1=	refont -c
#PR2=	| lp
#DUMMY=
#DOC=	doc/

#---- These settings are recommended for BSD 4.3 UNIX ----
#O=	.o
#E=
#EXTRA=
#EXTRA2=
#LIBS=	-ltermcap
#BIN=	/usr/local/bin
#CFLAGS=	-Dbsd -O
#OF=	-o 
#RF=	-c
#DATE=	-DDATE=\'\"`date`\"\'
#EVAL=	eval 
#PROGS=	elvis ctags ref virec refont
#CHMEM=	
#SORT=	-DSORT
#INST=	inst.unix
#RM=	rm -f
#PR1=	refont -c
#PR2=	| lpr
#DUMMY=
#DOC=	doc/

#---- These settings are recommended for Coherent ----
#O=.o
#E=
#EXTRA=
#EXTRA2=
#LIBS=	-lterm
#BIN=	/usr/bin
#CC=	cc 
#CFLAGS=	-O -DCOHERENT -DCRUNCH -DNO_CHARATTR -DNO_CURSORSHAPE \
#	-DNO_DIGRAPH -DNO_MKEXRC -VSUVAR
#OF=	-o 
#RF=	-c
#DATE=	-DDATE=\'\"`date`\"\'
#EVAL=	eval
#PROGS=	elvis ctags ref virec refont
#CHMEM=	fixstack 2000 elvis
#SORT=
#INST=	inst.unix
#RM=	rm -f
#PR1=	refont -b
#PR2=	| lpr
#DUMMY=
#DOC=	doc/

#---- These settings are recommended for Minix-ST ----
#O=	.o
#E=
#EXTRA=
#EXTRA2=
#LIBS=
#BIN=	/usr/bin
#CC=	cc
#CFLAGS=
#OF=	-o 
#RF=	-c
#DATE=	-DDATE=\'\"`date`\"\'
#EVAL=	eval 
#PROGS=	elvis ctags ref virec refont
#CHMEM=	chmem =18000 elvis
#SORT=
#INST=	inst.unix
#RM=	rm -f
#PR1=	lpr
#PR2=
#DUMMY=
#DOC=	doc/

#---- These settings are recommended for Minix-PC ----
#O=	.s
#E=
#EXTRA=	tinytcap.o
#EXTRA2=
#LIBS=
#BIN=	/usr/bin
#CC=	cc -i
#CFLAGS=	-O -DCRUNCH -DNO_MKEXRC -DNO_CURSORSHAPE -DNO_CHARATTR \
#	-DNO_SHOWMODE -DNO_MODELINE -DNO_OPTCOLS -DNO_DIGRAPH -DNO_ABBR \
#	-DNO_AT -DNO_SENTENCE -DNO_ERRLIST
#### (all but -DNO_EXTENSIONS, -DNO_RECYCLE, -DNO_MAGIC, and -DNO_CHARSEARCH)
#OF=	-o 
#RF=	-c
#DATE=	-DDATE=\'\"`date`\"\'
#EVAL=	eval 
#PROGS=	elvis ctags ref virec refont
#CHMEM=
#SORT=
#INST=	inst.unix
#RM=	rm -f
#PR1=	lpr
#PR2=	
#DUMMY=
#DOC=	doc/

#---- These settings are recommended for MS-DOS + MS-C + NDMAKE ----
#O=	.obj
#E=	.exe
#EXTRA=	pc.o sysdos.o tinytcap.o
#EXTRA2=
#LIBS=
#BIN=	c:\dos
#CC=	cl /AM
#CFLAGS=	-O -DCS_IBMPC -DCS_SPECIAL
#OF=	-o 
#RF=	-c
#DATE=
#EVAL=
#PROGS=	elvis ex ctags ref virec wildcard refont
#CHMEM=	
#SORT=
#INST=	inst.dos
#RM=	del
#PR1=	refont -c
#PR2=	>PRN
#DUMMY=
#DOC=	doc\

#---- These settings are recommended for Atari TOS + Mark Williams C ----
#O=.o
#E=.ttp
#EXTRA=	sysdos.o tinytcap.o
#EXTRA2=	atari.o
#LIBS=
#BIN=	c:\
#CC=	cc -VPEEP
#CFLAGS=	-O -DCS_IBMPC -DCS_SPECIAL
#OF=	-o 
#RF=	-c
#DATE=
#EVAL=
#PROGS=	elvis ctags ref virec wildcard shell refont
#CHMEM=
#SORT=
#INST=	inst.tos
#RM=	rm -f
#PR1=	refont -e
#PR2=	>PRT:
#DUMMY=
#DOC=	'doc\'

#---- These settings are recommended for OS-9/68K V2.3 ----
#O=	.r
#E=
#EXTRA=	date.o
#EXTRA2=	osk.o 
#LIBS=	-l=/dd/lib/termlib.l
#BIN=	/dd/usr/cmds
#CC=	cc
#ODIR=	/dd/usr/src/elvis
#CFLAGS=	-gq -m=2
#OF=	-f=$(ODIR)/
#RF=	-r
#DATE=
#EVAL=
#PROGS=	elvis vi view input ctags ref virec refont
#CHMEM=	touch date.r
#SORT=
#INST=	inst.os9
#RM=	del *.stb *.dbg
#PR1=	refont -b
#PR2=	>/p
#DUMMY=	dummy
#DOC=	doc/

###########################################################################
###########################################################################
###                                                                     ###
###     The rest of this Makefile contains no user-servicable parts     ###
###                                                                     ###
###########################################################################
###########################################################################

OBJS=	blk.o cmd1.o cmd2.o curses.o cut.o ex.o input.o main.o misc.o \
	modify.o move1.o move2.o move3.o move4.o move5.o opts.o recycle.o \
	redraw.o regexp.o regsub.o system.o tio.o tmp.o vars.o vcmd.o vi.o

ALIAS=	alias.o

DOCS=	$(DOC)index.doc $(DOC)intro.doc $(DOC)visual.doc $(DOC)ex.doc \
	$(DOC)regexp.doc $(DOC)options.doc $(DOC)cutbufs.doc $(DOC)differ.doc \
	$(DOC)internal.doc $(DOC)cflags.doc $(DOC)termcap.doc \
	$(DOC)environ.doc $(DOC)versions.doc

SRC1=	README KNOWN.BUGS $(DOC)intro.doc $(DOC)visual.doc $(DOC)ex.doc \
	$(DOC)versions.doc $(DOC)cflags.doc $(DOC)differ.doc
SRC2=	$(DOC)cutbufs.doc $(DOC)options.doc $(DOC)environ.doc $(DOC)regexp.doc \
	$(DOC)internal.doc $(DOC)termcap.doc $(DOC)index.doc $(DOC)ctags.man \
	$(DOC)elvis.man $(DOC)ref.man $(DOC)refont.man $(DOC)virec.man
SRC3=	Elvis.lnk Elvis.mak Elvis.prj Makefile.mix alias.c atari.c \
	ctags.c pc.c ref.c shell.c sysdos.c virec.c wildcard.c \
	profile.sh osk.c osk.h date.c
SRC4=	blk.c cmd1.c cmd2.c config.h curses.c
SRC5=	curses.h cut.c ex.c input.c main.c misc.c
SRC6=	modify.c move1.c move2.c move3.c move4.c move5.c opts.c recycle.c \
	redraw.c 
SRC7=	regexp.c regexp.h regsub.c system.c tinytcap.c tio.c tmp.c 
SRC8=	vars.c vcmd.c vi.c vi.h refont.c

###########################################################################

all: $(PROGS)

elvis: $(OBJS) $(EXTRA) $(EXTRA2)
	$(CC) -s $(CFLAGS) $(OF)elvis $(OBJS) $(EXTRA) $(EXTRA2) $(LIBS)
	$(CHMEM)

ctags: ctags.c
	$(CC) $(CFLAGS) $(SORT) $(OF)ctags ctags.c

ref: ref.c
	$(CC) $(CFLAGS) $(OF)ref ref.c

virec: virec.c
	$(CC) $(CFLAGS) $(OF)virec virec.c $(EXTRA2)

view: $(ALIAS)
	$(CC) $(CFLAGS) $(OF)view $(ALIAS)

ex: $(ALIAS)
	$(CC) $(CFLAGS) $(OF)ex $(ALIAS)

vi: $(ALIAS)
	$(CC) $(CFLAGS) $(OF)vi $(ALIAS)

input: $(ALIAS)
	$(CC) $(CFLAGS) $(OF)input $(ALIAS)

shell: shell.c
	$(CC) $(CFLAGS) $(OF)shell shell.c

wildcard: wildcard.c
	$(CC) $(CFLAGS) $(OF)wildcard wildcard.c

refont: refont.c
	$(CC) $(CFLAGS) $(OF)refont refont.c $(EXTRA2)

##############################################################################

# The file cmd1.c is compiled with the extra flag -DDATE="today's date".
#cmd1.o: cmd1.c vi.h config.h
#	$(EVAL) $(CC) $(CFLAGS) $(RF) $(DATE) cmd1.c

# "It all depends..."
$(OBJS): vi.h curses.h config.h

# OS9 must create a custom date.c file, and compile it.
date.o: $(OBJS)
	@echo '/* compilation date of elvis */' >-date.c
	@echo -r 'char date[] = "' >+date.c
	@echo -r 'echo -r ' >-/dd/tmp/date.c
	@date >+/dd/tmp/date.c
	@shell /dd/tmp/date.c >+date.c
	@echo '";' >+date.c
	@del /dd/tmp/date.c
	$(CC) $(CFLAGS) $(RF) date.c

##############################################################################
install: $(INST)
	@echo Installation complete.

inst.unix: $(DUMMY)
	$(INSTALL) -m 755 -o bin $(PROGS) $(BIN)
	$(LN) $(BIN)/elvis $(BIN)/vi
	$(LN) $(BIN)/elvis $(BIN)/ex
	$(LN) $(BIN)/elvis $(BIN)/view
	$(LN) $(BIN)/elvis $(BIN)/input

inst.dos: $(DUMMY)
	copy $(PROGS) $(BIN)
	copy $(BIN)/ex $(BIN)/vi
	copy $(BIN)/ex $(BIN)/view
	copy $(BIN)/ex $(BIN)/input

inst.tos: $(DUMMY)
	copy $(PROGS) $(BIN)

inst.os9: $(DUMMY)
	copy $(PROGS) -rw=$(BIN)
	chd $(BIN); attr -epenprnpw $(PROGS)

##############################################################################
clean: $(DUMMY)
	$(RM) *.o $(DOC)*.1 elvis?.uue elvis?.sh core elvis ctags ref virec refont


clobber: clean
	$(RM) tags refs $(PROGS)

##############################################################################
print: refont
	$(PR1) $(DOCS) $(PR2)

tags refs: ctags
	ctags -r *.c *.h

##############################################################################
uue: elvis1.uue elvis2.uue elvis3.uue elvis4.uue elvis5.uue \
	elvis6.uue elvis7.uue elvis8.uue

elvis1.uue: $(SRC1)
	tar cf elvis1.tar $(SRC1)
	compress -b13 elvis1.tar
	cp README elvis1.uue
	uue elvis1.tar.Z - >>elvis1.uue
	$(RM) elvis1.tar*

elvis2.uue: $(SRC2)
	tar cf elvis2.tar $(SRC2)
	compress -b13 elvis2.tar
	uue elvis2.tar.Z
	$(RM) elvis2.tar*

elvis3.uue: $(SRC3)
	tar cf elvis3.tar $(SRC3)
	compress -b13 elvis3.tar
	uuencode elvis3.tar.Z <elvis3.tar.Z >elvis3.uue
	$(RM) elvis3.tar*

elvis4.uue: $(SRC4)
	tar cf elvis4.tar $(SRC4)
	compress -b13 elvis4.tar
	uuencode elvis4.tar.Z <elvis4.tar.Z >elvis4.uue
	$(RM) elvis4.tar*

elvis5.uue: $(SRC5)
	tar cf elvis5.tar $(SRC5)
	compress -b13 elvis5.tar
	uuencode elvis5.tar.Z <elvis5.tar.Z >elvis5.uue
	$(RM) elvis5.tar*

elvis6.uue: $(SRC6)
	tar cf elvis6.tar $(SRC6)
	compress -b13 elvis6.tar
	uuencode elvis6.tar.Z <elvis6.tar.Z >elvis6.uue
	$(RM) elvis6.tar*

elvis7.uue: $(SRC7)
	tar cf elvis7.tar $(SRC7)
	compress -b13 elvis7.tar
	uuencode elvis7.tar.Z <elvis7.tar.Z >elvis7.uue
	$(RM) elvis7.tar*

elvis8.uue: $(SRC8)
	tar cf elvis8.tar $(SRC8)
	compress -b13 elvis8.tar
	uuencode elvis8.tar.Z <elvis8.tar.Z >elvis8.uue
	$(RM) elvis8.tar*

##############################################################################
sh: elvis1.sh elvis2.sh elvis3.sh elvis4.sh elvis5.sh elvis6.sh \
	elvis7.sh elvis8.sh

elvis1.sh: $(SRC1)
	cat   >elvis1.sh README
	echo >>elvis1.sh ': ------------------------ CUT HERE --------------------'
	echo >>elvis1.sh 'test -d doc || mkdir doc || exit 2'
	shar >>elvis1.sh -h $(SRC1)

elvis2.sh: $(SRC2)
	echo  >elvis2.sh ': ------------------------ CUT HERE --------------------'
	echo >>elvis2.sh 'test -d doc || mkdir doc || exit 2'
	shar >>elvis2.sh -h $(SRC2)

elvis3.sh: $(SRC3)
	shar $(SRC3) >elvis3.sh

elvis4.sh: $(SRC4)
	shar $(SRC4) >elvis4.sh

elvis5.sh: $(SRC5)
	shar $(SRC5) >elvis5.sh

elvis6.sh: $(SRC6)
	shar $(SRC6) >elvis6.sh

elvis7.sh: $(SRC7)
	shar $(SRC7) >elvis7.sh

elvis8.sh: $(SRC8)
	shar $(SRC8) >elvis8.sh

##############################################################################

# Under XENIX only!  This stores all sources on a 3.5" 720k floppy disk.
floppy: $(SRC1) $(SRC2) $(SRC3) $(SRC4) $(SRC5) $(SRC6) $(SRC7) $(SRC8)
	tar c5v $(SRC1) $(SRC2) $(SRC3) $(SRC4) $(SRC5) $(SRC6) $(SRC7) $(SRC8)
