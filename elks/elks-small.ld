OUTPUT_FORMAT(binary)

SECTIONS {
	.hdr : {
		SHORT (0x0301);  /* ELKS a.out magic */
		BYTE (0x30);     /* flags : executable with separated I & D */
		BYTE (0x04);     /* CPU : i8086 */
		BYTE (0x20);     /* header length (32) */
		BYTE (0);        /* unused */
		SHORT (0);       /* version (unused) */
		LONG(SIZEOF (.text));
		LONG(SIZEOF (.data));
		LONG (SIZEOF (.bss));
		LONG (_start);    /* entry point */
		LONG (_total_adjusted); /* total memory allocated */
		LONG (0);        /* symbol table size */
		}
	_begintext = ALIGN(0x20);
	.text 0 : AT(_begintext) { *(.text*); . = ALIGN(0x10); }
	_begindata = _begintext + .;
	.data 0 : AT(_begindata) { *(.nildata*) *(.rodata*) *(.data*) }
	.bss : {
		*(.bss) *(COMMON)
		ASSERT (. + 0x100 <= 0x10000,
		    "Error: too large for a small-model ELKS a.out file.");
	}
	PROVIDE (_start = entry); /* `entry' was the old entry point symbol */
	PROVIDE (_chmem = 0);
	PROVIDE (_total = _chmem == 0 ? 0 : . + _chmem);
	_total_adjusted = _total == 0 ? 0
			  : MIN (0x10000, MAX (. + 0x100, _total));
	/DISCARD/ : { *(.comment) }
}
