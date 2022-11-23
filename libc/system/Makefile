SYSCALLDAT=${TOPDIR}/elks/arch/i86/kernel/syscall.dat

ifeq "$(VPATH)" ""
	dir	=
else
	dir	= $(VPATH)/
endif

all: out.a

out.a: syscall.mk out

syscall.mk: $(SYSCALLDAT) $(dir)syscall.awk
	awk -f $(dir)syscall.awk $< > $@

out::
	$(MAKE) -f $(dir)out.mk

clean::
	touch syscall.mk
	$(MAKE) -f $(dir)out.mk clean
	$(RM) syscall.mk out
	$(RM) -r syscall
