OUTPUT_FORMAT(binary)

SECTIONS {
	.text : { *(.text*) *(.rodata*) *(.data*) }
	.bss : { *(.bss) *(COMMON) }
	/DISCARD/ : { *(.comment) }
}
