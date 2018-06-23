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
		LONG (entry);    /* entry point */
		LONG (0);        /* total memory allocated */
		LONG (0);        /* symbol table size */
		}
	_begintext = ALIGN(0x20);
	.text 0 : AT(_begintext) { *(.text*); . = ALIGN(0x10); }
	_begindata = _begintext + .;
	.data 0 : AT(_begindata) { *(.data*) *(.rodata*) }
	.bss : { *(.bss) *(COMMON) }
	/DISCARD/ : { *(.comment) }
}
