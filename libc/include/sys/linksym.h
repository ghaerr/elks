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
//#define __LINK_SYMBOL(sym)  static void __based(__segname("DISCARD")) (*__sym)() = { sym }
//#define __LINK_SYMBOL(sym)  static void (* __CONCAT(__linksym,sym))() = { sym }
extern unsigned int __linksym;
#define __LINK_SYMBOL(sym)    __linksym = (unsigned)sym     /* FIXME use better method */
#endif

#else  /* __ASSEMBLER__ */

#define __LINK_SYMBOL(sym) \
                        .pushsection .discard; \
                        .long #sym; \
                        .popsection
#endif  /* __ASSEMBLER__ */

#endif
