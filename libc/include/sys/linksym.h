#ifndef __SYS_LINKSYM_H
#define __SYS_LINKSYM_H
/* Internal macro to force a reference to an external symbol */
/* Partly based on https://justine.lol/sizetricks/ */
/* This assumes that the linker script discards any .discard sections */

#ifndef __ASSEMBLER__

#ifdef __GNUC__
#define __LINK_SYMBOL(sym) \
                          asm(".pushsection .discard; " \
                              ".long " #sym "; " \
                              ".popsection")
#endif

#ifdef __WATCOMC__
extern void __LINK_SYMBOL(void (*sym)());
#pragma aux __LINK_SYMBOL = __parm [ __ax];
#endif

#else  /* __ASSEMBLER__ */

#define __LINK_SYMBOL(sym) \
                        .pushsection .discard; \
                        .long #sym; \
                        .popsection
#endif  /* __ASSEMBLER__ */

#endif
