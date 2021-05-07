ENTRY(_start)

SECTIONS {
	.text 0x10000 : {
		/* IA-16 segment start markers. */
		*(".text!*" ".text.*!")
		*(".rodata!*" ".rodata.*!")
		*(".data!*" ".data.*!")
		*(".bss!*" ".bss.*!")

		/* Actual segment contents. */
		*(.text .text$* ".text.*[^&]")
		*(.rodata .rodata$* ".rodata.*[^&]")
		*(.data .data$* ".data.*[^&]")
		*(.bss .bss$* ".bss.*[^&]")
		*(COMMON)

		/* IA-16 segment end markers. */
		*(".text&*" ".text.*&")
		*(".nildata&*" ".nildata.*&")
		*(".rodata&*" ".rodata.*&")
		*(".data&*" ".data.*&")
		*(".bss&*" ".bss.*&")

		ASSERT (. + 0x100 - ADDR (.text) <= 0xfff0,
		    "Error: too large for a tiny-model ELKS a.out file.");
	}

	.rel.dyn : {
		*(.rel.*)
	}

	/DISCARD/ : { *(.comment) }

	/* `entry' was the old entry point symbol.  (Unfortunately this
	   PROVIDE does not seem to work with the ld.gold linker --- it did
	   work with ld.bfd.  FIXME?)  */
	PROVIDE (_start = DEFINED (entry) ? entry : 0);
}
