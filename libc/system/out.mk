include $(TOPDIR)/libc/Makefile.inc

DEFINES	+= -DL_execl -DL_execle -DL_execlp -DL_execlpe \
	   -DL_sleep -DL_usleep

ifneq "$(VPATH)" ""
	dir	= $(VPATH)/
else
	dir	=
endif

OBJS = \
	abort.o \
	argcargv.o \
	closedir.o \
	dup.o \
	dup2.o \
	environ.o \
	errno.o \
	execl.o \
	execle.o \
	execlp.o \
	execlpe.o \
	execv.o \
	execve.o \
	execvp.o \
	execvpe.o \
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
	opendir.o \
	program_filename.o \
	readdir.o \
	rewinddir.o \
	seekdir.o \
	setjmp.o \
	setpgrp.o \
	signal.o \
	sleep.o \
	syscall01.o \
	syscall23.o \
	syscall4.o \
	syscall5.o \
	signalcb.o \
	telldir.o \
	time.o \
	times.o \
	usleep.o \
	wait.o \
	wait3.o \
	waitpid.o \

include syscall.mk

all: out.a
.PHONY: all

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	$(RM) *.[aod]
.PHONY: clean
