ENTRY(_start)

SECTIONS {
	.text 0x10000 : {
		/* IA-16 segment start markers. */
		*(".text!*" ".text.*!"
		  ".fartext.f.*0!" ".fartext.f.*2!"
		  ".fartext.f.*4!" ".fartext.f.*6!")

		/* Actual segment contents. */
		*(.text .text$* ".text.*[^&]"
		  /*
		   * Place the text segments from about half the input
		   * modules into the near text segment.  Let the rest go
		   * into .fartext .
		   */
		  .fartext.f.*0$ .fartext.f.*2$
		  .fartext.f.*4$ .fartext.f.*6$)

		/* IA-16 segment end markers. */
		*(".text&*" ".text.*&"
		  ".fartext.f.*0&" ".fartext.f.*2&"
		  ".fartext.f.*4&" ".fartext.f.*6&")
		. = ALIGN(0x10);
	}

	.fartext 0x20000 : {
		/* IA-16 segment start markers. */
		*(".fartext!*" ".fartext.*!")

		/* Actual segment contents. */
		*(.fartext .fartext$ ".fartext.*[^&]")

		/* IA-16 segment end markers. */
		*(".fartext&*" ".fartext.*&")
		. = ALIGN(0x10);
	}

	.data 0x30000 : {
		/* IA-16 segment start markers. */
		*(".nildata!*" ".nildata.*!")
		*(".rodata!*" ".rodata.*!")
		*(".data!*" ".data.*!")
		*(".bss!*" ".bss.*!")

		/* Actual segment contents. */
		*(.nildata .nildata$* ".nildata.*[^&]")
		*(.rodata .rodata$* ".rodata.*[^&]")
		*(.data .data$* ".data.*[^&]")
		. = ALIGN(0x10);
	}

	.bss : {
		*(.bss .bss$* ".bss.*[^&]")
		*(COMMON)

		/* IA-16 segment end markers. */
		*(".nildata&*" ".nildata.*&")
		*(".rodata&*" ".rodata.*&")
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
}
