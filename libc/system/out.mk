include $(TOPDIR)/libc/Makefile.inc

ifneq "$(VPATH)" ""
	dir	= $(VPATH)/
else
	dir	=
endif

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
	time.o \
	times.o \
	usleep.o \
	wait.o \
	wait3.o \
	waitpid.o \

include syscall.mk

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean::
	$(RM) $(OBJS)
