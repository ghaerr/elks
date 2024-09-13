# Sub-Makefile of /libc/system module

COMPILER ?= ia16
LIB ?= out.a

include $(TOPDIR)/libc/$(COMPILER).inc

DEFINES	+= -DL_execl -DL_execle -DL_execlp -DL_execlpe \
	   -DL_sleep -DL_usleep

ifneq "$(VPATH)" ""
	dir	= $(VPATH)/
else
	dir	=
endif

OBJS = \
	abort.o \
	closedir.o \
	dup.o \
	dup2.o \
	errno.o \
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
	readdir.o \
	rewinddir.o \
	seekdir.o \
	setpgrp.o \
	sigaction.o \
	signal.o \
	sleep.o \
	telldir.o \
	time.o \
	times.o \
	usleep.o \
	wait.o \
	wait3.o \
	waitpid.o \

# these files written in assembly language
IA16OBJS = \
	argcargv.o \
	environ.o \
	execl.o \
	execle.o \
	execlp.o \
	execlpe.o \
	program_filename.o \
	setjmp.o \
	syscall01.o \
	syscall23.o \
	syscall4.o \
	syscall5.o \
	signalcb.o \

ifeq "$(COMPILER)" "ia16"
OBJS += $(IA16OBJS)
include syscall.mk
endif

all: $(LIB)
.PHONY: all

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean:
	$(RM) *.[aod]
.PHONY: clean
