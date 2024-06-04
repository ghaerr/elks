#ifndef __SYS_WEAKEN_H
#define __SYS_WEAKEN_H

#ifdef __GNUC__
/* return address of weak version of passed symbol */
#define _weaken(sym) __extension__ ({                           \
        extern __typeof__(sym) sym __attribute__((__weak__));   \
        sym; })
#endif

#ifdef __WATCOMC__
#define _weaken(sym)    (sym)   /* FIXME weak symbols not yet implemented */
#endif

#endif
