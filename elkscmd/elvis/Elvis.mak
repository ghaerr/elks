# Makefile for MSC - if you don't have NDmake, use this one,
# but don't expect to be happy.
# And don't expect to do anything but making the executables, either.

OBJS=	blk.obj cmd1.obj cmd2.obj curses.obj cut.obj ex.obj input.obj \
	main.obj misc.obj modify.obj move1.obj move2.obj move3.obj move4.obj \
	move5.obj opts.obj recycle.obj redraw.obj regexp.obj regsub.obj \
	system.obj tio.obj tmp.obj vars.obj vcmd.obj vi.obj \
	pc.obj sysdos.obj tinytcap.obj

CFLAGS=	-DCS_IBMPC -DCS_SPECIAL
CC=	cl -AM
	
blk.obj:	blk.c
	$(CC) $(CFLAGS) -c blk.c

cmd1.obj:	cmd1.c
	$(CC) $(CFLAGS) -c cmd1.c

cmd2.obj:	cmd2.c
	$(CC) $(CFLAGS) -c cmd2.c

curses.obj:	curses.c
	$(CC) $(CFLAGS) -c curses.c

cut.obj:	cut.c
	$(CC) $(CFLAGS) -c cut.c

ex.obj:		ex.c
	$(CC) $(CFLAGS) -c ex.c

input.obj:	input.c
	$(CC) $(CFLAGS) -c input.c

main.obj:	main.c
	$(CC) $(CFLAGS) -c main.c

misc.obj:	misc.c
	$(CC) $(CFLAGS) -c misc.c

modify.obj:	modify.c
	$(CC) $(CFLAGS) -c modify.c

move1.obj:	move1.c
	$(CC) $(CFLAGS) -c move1.c

move2.obj:	move2.c
	$(CC) $(CFLAGS) -c move2.c

move3.obj:	move3.c
	$(CC) $(CFLAGS) -c move3.c

move4.obj:	move4.c
	$(CC) $(CFLAGS) -c move4.c

move5.obj:	move5.c
	$(CC) $(CFLAGS) -c move5.c

opts.obj:	opts.c
	$(CC) $(CFLAGS) -c opts.c

recycle.obj:	recycle.c
	$(CC) $(CFLAGS) -c recycle.c

redraw.obj:	redraw.c
	$(CC) $(CFLAGS) -c redraw.c

regexp.obj:	regexp.c
	$(CC) $(CFLAGS) -c regexp.c

regsub.obj:	regsub.c
	$(CC) $(CFLAGS) -c regsub.c

system.obj:	system.c
	$(CC) $(CFLAGS) -c system.c

tio.obj:	tio.c
	$(CC) $(CFLAGS) -c tio.c

tmp.obj:	tmp.c
	$(CC) $(CFLAGS) -c tmp.c

vars.obj:	vars.c
	$(CC) $(CFLAGS) -c vars.c

vcmd.obj:	vcmd.c
	$(CC) $(CFLAGS) -c vcmd.c

vi.obj:		vi.c
	$(CC) $(CFLAGS) -c vi.c

pc.obj:		pc.c
	$(CC) $(CFLAGS) -c pc.c

sysdos.obj:	sysdos.c
	$(CC) $(CFLAGS) -c sysdos.c

tinytcap.obj:	tinytcap.c
	$(CC) $(CFLAGS) -c tinytcap.c

elvis.exe: $(OBJS)
	link @elvis.lnk

ctags.exe: ctags.c wildcard.c
	$(CC) ctags.c -o ctags.exe

ref.exe: ref.c
	$(CC) ref.c -o ref.exe

virec.exe: virec.c wildcard.c
	$(CC) virec.c -o virec.exe

wildcard.exe: wildcard.c
	$(CC) wildcard.c -o wildcard.exe

ex.exe: alias.c
	$(CC) alias.c -o ex.exe

vi.exe: ex.exe
	copy ex.exe vi.exe

view.exe: ex.exe
	copy ex.exe view.exe
