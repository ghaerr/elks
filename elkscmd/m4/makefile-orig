#
# pd m4	[oz]
#
#	-DEXTENDED 
#		if you like to get paste & spaste macros.
#	-DVOID 
#		if your C compiler does NOT support void.
#	-DGETOPT
#		if you STILL do not have getopt	in your library.
#		[This means your library is broken. Fix it.]
#	-DDUFFCP
#		if you do not have fast memcpy in your library.
#
include ../Make.defs

CFLAGS = $(CFLBASE) -DEXTENDED
DEST =  /usr/local/bin
MANL = 	/usr/man/manl
OBJS =  main.o eval.o serv.o look.o misc.o expr.o
CSRC =  main.c eval.c serv.c look.c misc.c expr.c
INCL =  mdef.h extr.h
MSRC =  ack.m4 hanoi.m4 hash.m4 sqroot.m4 string.m4 test.m4
DOCS =	README MANIFEST m4.1

MBIN = /usr/bin

m4: ${OBJS}
	@echo "loading m4.."
	$(CC) -s -o m4 ${OBJS}
	size86 m4

${OBJS}: ${INCL} 

lint:
	lint -h ${CSRC}

install: m4
	install ./m4 ${DEST}/m4
	cp ./m4.1 ${MANL}/m4.l

deinstall: 
	rm -f ${DEST}/m4
	rm -f ${MANL}/m4.l
time: m4
	@echo "timing comparisons.."
	@echo "un*x m4:"
	time ${MBIN}/m4 <test.m4 >unxm4.out
	@echo "pd m4:"
	time ./m4 <test.m4 >pdm4.out
	@echo "un*x m4:"
	time ${MBIN}/m4 <test.m4 >unxm4.out
	@echo "pd m4:"
	time ./m4 <test.m4 >pdm4.out
	@echo "un*x m4:"
	time ${MBIN}/m4 <test.m4 >unxm4.out
	@echo "pd m4:"
	time ./m4 <test.m4 >pdm4.out
	@echo "output comparisons.."
	-diff pdm4.out unxm4.out
	@rm -f pdm4.out unxm4.out
clean:
	rm -f *.o core m4 *.out
pack:
	shar -a makefile ${INCL} ${CSRC} >M4MAIN.SHAR
	shar -a ${MSRC} ${DOCS} >M4MSRC.SHAR
