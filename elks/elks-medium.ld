OUTPUT_FORMAT(elks)
ENTRY(_start)

SECTIONS {
	.text 0 : {
		CREATE_OBJECT_SYMBOLS
		*(.text .text.*
		  /*
		   * Place the text segments from about half the input
		   * modules into the near text segment.  Let the rest go
		   * into .fartext .
		   */
		  .fartext.f.*0$ .fartext.f.*2$ .fartext.f.*4$ .fartext.f.*6$)
		. = ALIGN(0x10);
	}
	.fartext 0 : AT(0x10000) {
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
		    "Error: too large for a medium-model ELKS a.out file.");
	}
	/DISCARD/ : { *(*) }
}
