# Makefile of /libc/syscall module

include $(TOPDIR)/libc/Makefile.inc

SYSCALLDAT=${TOPDIR}/elks/arch/i86/kernel/syscall.dat

OBJS = syscall0.o syscall1.o syslib.o signal.o exec.o dirent.o setjmp.o

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

syscall1.S: syscall.awk $(SYSCALLDAT)
	tr '[A-Z]' '[a-z]' < $(SYSCALLDAT) > syscall.dat
	awk -f $< syscall.dat > syscall1.S

syscall0.o: syscall0.S
syscall1.o: syscall1.S
syslib.o: syslib.c
signal.o: signal.c
exec.o: exec.c
setjmp.o: setjmp.S
#setjmp.s: setjmp.S

clean:
	rm -f *.[ao]
	rm -f syscall.dat syscall1.s
	rm -f call_tab.v defn_tab.v
