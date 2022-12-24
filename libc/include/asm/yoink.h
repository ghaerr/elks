/* Internal macro to force a reference to an external symbol */
/* Partly based on https://justine.lol/sizetricks/ */
/* This assumes that the linker script discards any .discard sections */

#pragma once

#ifndef __ASSEMBLER__
# define __YOINK(sym)	__asm(".pushsection .discard; " \
			      ".long " __YOINK_STR(#sym) "; " \
			      ".popsection")
# define __YOINK_STR(sym) #sym
#else  /* __ASSEMBLER__ */
# define __YOINK(sym)	.pushsection .discard; \
			.long #sym; \
			.popsection
#endif  /* __ASSEMBLER__ */
