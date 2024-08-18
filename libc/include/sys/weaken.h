#ifndef __SYS_WEAKEN_H
#define __SYS_WEAKEN_H

#ifdef __GNUC__
/* return address of weak version of passed symbol */
#define _weakaddr(sym) __extension__ ({                         \
        extern __typeof__(sym) sym __attribute__((__weak__));   \
        sym; })

#define _weakfn(sym) __extension__ ({                           \
        extern __typeof__(sym) sym __attribute__((__weak__));   \
        sym; })
#endif

#ifdef __WATCOMC__

#if defined(__COMPACT__) || defined(__LARGE__)
/* FIXME: this can fail if weak symbol happens to fall at address zero of segment! */
#define _weakaddr(sym)  (((unsigned long)sym) & 0xFFFF)
#else
#define _weakaddr(sym)  (sym)
#endif

#define _weakfn(sym)    (sym)
#endif

#endif
