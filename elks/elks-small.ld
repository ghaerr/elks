OUTPUT_FORMAT(elks)
ENTRY(_start)

SECTIONS {
	.text 0 : {
		CREATE_OBJECT_SYMBOLS
		*(.text .text.*)
		. = ALIGN(0x10);
	}
	.fartext 0 : AT(0x10000) {
		/*
		 * Some small-model programs might want to break out of the
		 * small memory model a bit --- and choose to place some of
		 * their code in a far segment outside the default text
		 * segment.  Allow this.
		 */
		*(.fartext .fartext$ .fartext.*)
		. = ALIGN(0x10);
	}
	.data 0 : AT(0x20000) {
		*(.nildata .nildata.*)
		*(.rodata .rodata.*)
		*(.data .data.*)
		CONSTRUCTORS
		. = ALIGN(0x10);
	}
	.bss : {
		*(.bss .bss.*)
		*(COMMON)
		. = ALIGN(0x10);
		ASSERT (. + 0x100 <= 0xfff0,
		    "Error: too large for a small-model ELKS a.out file.");
		/* Sanity check any -maout-total= and -maout-chmem= values */
		PROVIDE (_total = 0);
		PROVIDE (_chmem = 0);
		ASSERT (_total <= 0xfff0 && . + _chmem <= 0xfff0,
		    "Error: total data segment size too large.");
		ASSERT ((_total == 0 || _total > .) && _chmem >= 0,
		    "Error: total data segment size too small.");
	}
	/* `entry' was the old entry point symbol */
	PROVIDE (_start = DEFINED (entry) ? entry : 0);
	/DISCARD/ : { *(*) }
}
