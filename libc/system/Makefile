SYSCALLDAT=${TOPDIR}/elks/arch/i86/kernel/syscall.dat

ifeq "$(VPATH)" ""
	dir	=
else
	dir	= $(VPATH)/
endif
ifeq "$(filter -mregparmcall,$(CC) $(CFLAGS) $(MULTILIB))" ""
	regparmcall = 0
else
	regparmcall = 1
endif

all: out.a
.PHONY: all

out.a: syscall.mk out

syscall.mk: $(SYSCALLDAT) $(dir)syscall.awk
	awk -f $(dir)syscall.awk regparmcall=$(regparmcall) $< > $@

out: syscall.mk
	$(MAKE) -f $(dir)out.mk
.PHONY: out

clean:
	touch syscall.mk
	$(MAKE) -f $(dir)out.mk clean
	$(RM) syscall.mk out
	$(RM) -r syscall
.PHONY: clean
