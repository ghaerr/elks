/* Internal macro to force a reference to an external symbol */
/* Partly based on https://justine.lol/sizetricks/ */
/* This assumes that the linker script discards any .discard sections */

#pragma once

#ifndef __ASSEMBLER__

#ifdef __MEDIUM__
#define _weakaddr(symbolstr) __extension__ ({   \
    unsigned int a, d;                          \
    asm(".weak\t"  symbolstr "\n"               \
        "\tmov\t$" symbolstr ",%%ax\n"          \
        "\tmov\t$" symbolstr "@OZSEG16,%%dx\n"  \
            : "=a" (a), "=d" (d)                \
            /* no input */ );                   \
    ((unsigned long)d << 16) | a;               \
    })

#else

#define _weakaddr(symbolstr) __extension__ ({   \
    unsigned int a;                             \
    asm(".weak\t"  symbolstr "\n"               \
        "\tmov\t$" symbolstr ",%%ax\n"          \
            : "=a" (a)                          \
            /* no input */ );                   \
    a;                                          \
    })
#endif

#define _weaken(symbol)       ((const typeof(&(symbol)))_weakaddr(#symbol))

#define __YOINK(sym)	__asm(".pushsection .discard; " \
                              ".long " __YOINK_STR(#sym) "; " \
                              ".popsection")
#define __YOINK_STR(sym) #sym

#else  /* __ASSEMBLER__ */
#define __YOINK(sym)	.pushsection .discard; \
                        .long #sym; \
                        .popsection
#endif  /* __ASSEMBLER__ */
