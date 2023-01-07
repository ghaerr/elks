/* Internal macro to force a reference to an external symbol */
/* Partly based on https://justine.lol/sizetricks/ */
/* This assumes that the linker script discards any .discard sections */

#pragma once

#ifndef __ASSEMBLER__

#define __STRING(x)   #x

/* generate new external symbol with same name and type as symbol but weak */
#define _weaken(sym)   ({ \
                          extern __typeof__(sym) __weak_sym \
                             __asm(__STRING(sym)) \
                             __attribute__((__weak__)); \
                          __weak_sym; \
                       })

#define __YOINK(sym)	__asm(".pushsection .discard; " \
                              ".long " __STRING(#sym) "; " \
                              ".popsection")
#else  /* __ASSEMBLER__ */
#define __YOINK(sym)	.pushsection .discard; \
                        .long #sym; \
                        .popsection
#endif  /* __ASSEMBLER__ */
