/*
 *	NO BIOS
 */

#include <linuxmt/config.h>

	.arch	i8086, nojumps
	.code16
	.text
	.global	call_bios

call_bios:
	ret
