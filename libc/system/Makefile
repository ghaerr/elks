# Makefile of /libc/syscall module

include $(TOPDIR)/libc/Makefile.inc

SYSCALLDAT=${TOPDIR}/elks/arch/i86/kernel/syscall.dat

OBJS = syscall0.o syscall1.o syslib.o signal.o exec.o dirent.o setjmp.o

all: $(LIBC) 

$(LIBC): $(OBJS)
	$(AR) $(ARFLAGS) $@ $?

syscall1.s: syscall.awk $(SYSCALLDAT)
	tr '[A-Z]' '[a-z]' < $(SYSCALLDAT) > syscall.dat
	awk -f syscall.awk syscall.dat > syscall1.s

syscall0.o: syscall0.s
syscall1.o: syscall1.s
syslib.o: syslib.c
signal.o: signal.c
exec.o: exec.c
setjmp.o: setjmp.s
setjmp.s: setjmp.S

clean:
	rm -f *.o setjmp.s
	rm -f syscall.dat syscall1.s
	rm -f call_tab.v defn_tab.v
