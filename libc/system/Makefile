# Makefile of /libc/syscall module

include $(TOPDIR)/libc/Makefile.inc

SYSCALLDAT=${TOPDIR}/elks/arch/i86/kernel/syscall.dat

OBJS = \
	abort.o \
	cleanup.o \
	dirent.o \
	dup.o \
	dup2.o \
	environ.o \
	errno.o \
	exec.o \
	getegid.o \
	geteuid.o \
	getgid.o \
	getpgid.o \
	getpid.o \
	getppid.o \
	getuid.o \
	killpg.o \
	lseek.o \
	mkfifo.o \
	setjmp.o \
	setpgrp.o \
	signal.o \
	sleep.o \
	syscall0.o \
	syscall1.o \
	time.o \
	times.o \
	usleep.o \
	wait.o \
	wait3.o \
	waitpid.o \

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

syscall1.s: syscall.awk $(SYSCALLDAT)
	tr '[A-Z]' '[a-z]' < $(SYSCALLDAT) > syscall.dat
	awk -f $< syscall.dat > syscall1.s

syscall0.o: syscall0.s
syscall1.o: syscall1.s
signal.o: signal.c
exec.o: exec.c
setjmp.o: setjmp.S
#setjmp.s: setjmp.S

clean:
	rm -f *.[ao]
	rm -f syscall.dat syscall1.s
	rm -f call_tab.v defn_tab.v
