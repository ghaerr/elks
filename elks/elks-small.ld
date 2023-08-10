ENTRY(_start)

SECTIONS {
	.text 0x10000 : {
		/* IA-16 segment start markers. */
		KEEP (*(".inithead!"))
		KEEP (*(SORT (".preinit!*") SORT (".preinit.*!")))
		KEEP (*(SORT (".init!*") SORT (".init.*!")))
		KEEP (*(SORT (".postinit!*") SORT (".postinit.*!")))
		KEEP (*(".inittail!"))
		*(".text!*" ".text.*!")
		KEEP (*(".finihead!"))
		KEEP (*(SORT (".fini!*") SORT (".fini.*!")))
		KEEP (*(".finitail!"))

		/* Actual segment contents. */
		KEEP (*(.inithead .inithead$))
		KEEP (*(.preinit SORT (.preinit$*) SORT (".preinit.*[^&]")))
		KEEP (*(.init SORT (.init$*) SORT (".init.*[^&]")))
		KEEP (*(.postinit SORT (.postinit$*) SORT (".postinit.*[^&]")))
		KEEP (*(.inittail .inittail$))
		*(.text .text$* ".text.*[^&]")
		KEEP (*(.finihead .finihead$))
		KEEP (*(.fini SORT (.fini$*) SORT (".fini.*[^&]")))
		KEEP (*(.finitail .finitail$))

		/* IA-16 segment end markers. */
		KEEP (*(".inithead&"))
		KEEP (*(SORT (".preinit&*") SORT (".preinit.*&")))
		KEEP (*(SORT (".init&*") SORT (".init.*&")))
		KEEP (*(SORT (".postinit&*") SORT (".postinit.*&")))
		KEEP (*(".inittail&"))
		*(".text&*" ".text.*&")
		KEEP (*(".finihead&"))
		KEEP (*(SORT (".fini&*") SORT (".fini.*&")))
		KEEP (*(".finitail&"))
		. = ALIGN(0x10);
	}

	.fartext 0x20000 : {
		/*
		 * Some small-model programs might want to break out of the
		 * small memory model a bit --- and choose to place some of
		 * their code in a far segment outside the default text
		 * segment.  Allow this.
		 *
		 * IA-16 segment start markers.
		 */
		*(".fartext!*" ".fartext.*!")
		"__start_fartext_init!" = .;
		"__end_fartext_init!" = .;

		/* Actual segment contents. */
		/* *(.fartext .fartext$* ".fartext.*[^&]") */

		*(.fartext .fartext.
		".fartext.[^i]*" ".fartext.i[^n]*" ".fartext.in[^i]*" ".fartext.ini[^t]*")
		 __start_fartext_init = .;
		*(.fartext .fartext.init)
		__end_fartext_init = .;

		/* IA-16 segment end markers. */
		*(".fartext&*" ".fartext.*&")
		. = ALIGN(0x10);
	}

	.data 0x30000 : {
		/* IA-16 segment start markers. */
		*(".nildata!*" ".nildata.*!")
		*(".rodata!*" ".rodata.*!")
		KEEP (*(".data.__CTOR_LIST__!"))
		KEEP (*(SORT (".ctors!*") SORT (".ctors.*!")))
		KEEP (*(".data.__CTOR_END__!"))
		KEEP (*(".data.__DTOR_LIST__!"))
		KEEP (*(SORT (".dtors!*") SORT (".dtors.*!")))
		KEEP (*(".data.__DTOR_END__!"))
		*(".data!*" ".data.*!")
		*(".bss!*" ".bss.*!")

		/* Actual segment contents. */
		*(.nildata .nildata$* ".nildata.*[^&]")
		*(.rodata .rodata$* ".rodata.*[^&]")
		KEEP (*(.data.__CTOR_LIST__ .data.__CTOR_LIST__$))
		KEEP (*(.ctors SORT (.ctors$*) SORT (".ctors.*[^&]")))
		KEEP (*(.data.__CTOR_END__ .data.__CTOR_END__$))
		KEEP (*(.data.__DTOR_LIST__ .data.__DTOR_LIST__$))
		KEEP (*(.dtors SORT (.dtors$*) SORT (".dtors.*[^&]")))
		KEEP (*(.data.__DTOR_END__ .data.__DTOR_END__$))
		*(.data .data$* ".data.*[^&]")
		. = ALIGN(0x10);
	}

	.bss : {
		*(.bss .bss$* ".bss.*[^&]")
		*(COMMON)

		/* IA-16 segment end markers. */
		*(".nildata&*" ".nildata.*&")
		*(".rodata&*" ".rodata.*&")
		KEEP (*(".data.__CTOR_LIST__&"))
		KEEP (*(SORT (".ctors&*") SORT (".ctors.*&")))
		KEEP (*(".data.__CTOR_END__&"))
		KEEP (*(".data.__DTOR_LIST__&"))
		KEEP (*(SORT (".dtors&*") SORT (".dtors.*&")))
		KEEP (*(".data.__DTOR_END__&"))
		*(".data&*" ".data.*&")
		*(".bss&*" ".bss.*&")
		. = ALIGN(0x10);

		ASSERT (. + 0x100 - ADDR (.data) <= 0xfff0,
		    "Error: too large for a small-model ELKS a.out file.");
		/* Sanity check any -maout-total= and -maout-chmem= values */
		PROVIDE (_total = 0);
		PROVIDE (_chmem = 0);
		ASSERT (_total <= 0xfff0
			&& . - ADDR (.data) + _chmem <= 0xfff0,
		    "Error: total data segment size too large.");
		ASSERT ((_total == 0 || _total > . - ADDR (.data))
			&& _chmem >= 0,
		    "Error: total data segment size too small.");
	}

	.rel.dyn : {
		*(.rel.*)
	}

	/DISCARD/ : {
		*(".discard!*" .discard ".discard&*")
	}

	/* `entry' was the old entry point symbol.  (Unfortunately this
	   PROVIDE does not seem to work with the ld.gold linker --- it did
	   work with ld.bfd.  FIXME?)  */
	PROVIDE (_start = DEFINED (entry) ? entry : 0);
}
