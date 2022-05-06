	.arch	i8086, nojumps
	.code16
	.text

	.global intc5_handler

intc5_handler:
	iret
