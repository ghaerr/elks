/* Internal macro to force a reference to an external symbol */
/* Partly based on https://justine.lol/sizetricks/ */
/* This assumes that the linker script discards any .discard sections */

#pragma once

#ifndef __ASSEMBLER__

#define _ezlea(symbol) "mov\t$" symbol ",%k"

#define _weakaddr(symbolstr)                                             \
  ({                                                                     \
    intptr_t waddr;                                                      \
    asm(".weak\t" symbolstr "\n\t" _ezlea(symbolstr) "0" : "=r"(waddr)); \
    waddr;                                                               \
  })

#define _strongaddr(symbolstr)                \
  ({                                          \
    intptr_t waddr;                           \
    asm(_ezlea(symbolstr) "0" : "=r"(waddr)); \
    waddr;                                    \
  })

#define _weaken(symbol)       ((const typeof(&(symbol)))_weakaddr(#symbol))
#define _strong(symbol)       ((const typeof(&(symbol)))_strongaddr(#symbol))

#define __YOINK(sym)	__asm(".pushsection .discard; " \
                              ".long " __YOINK_STR(#sym) "; " \
                              ".popsection")
#define __YOINK_STR(sym) #sym

#else  /* __ASSEMBLER__ */
#define __YOINK(sym)	.pushsection .discard; \
                        .long #sym; \
                        .popsection
#endif  /* __ASSEMBLER__ */
