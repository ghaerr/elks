#ifndef __ALLOCA_H
#define __ALLOCA_H
/*
 * Stack-checking alloca for GCC, OWC and C86
 */

int __stackavail(unsigned int size);    /* return 1 if size bytes stack available */
extern unsigned int __stacklow;         /* lowest protected SP value */

#ifdef __GNUC__
#define alloca(s)           (__stackavail(__ALLOCA_ALIGN(s))?   \
                                 __alloca(__ALLOCA_ALIGN(s)): (void *)0)
#define __ALLOCA_ALIGN(s)   (((s)+(sizeof(int)-1))&~(sizeof(int)-1))

/* The compiler alloca auto-aligns the stack from the parameter somewhat strangely:
 * 0 -> 0, 1 -> 2, 2 -> 4, 3 -> 4, 4 -> 6 etc.
 * Thus, __stackavail should check for two more bytes available than asked for.
 */
#define __alloca(s)         __builtin_alloca(s)
#endif

#ifdef __WATCOMC__
#define alloca(s)           (__stackavail(__ALLOCA_ALIGN(s))?   \
                                 __alloca(__ALLOCA_ALIGN(s)): (void *)0)
#define __ALLOCA_ALIGN(s)   (((s)+(sizeof(int)-1))&~(sizeof(int)-1))

#pragma aux __stackavail "*" __modify __nomemory

extern void __based(__segname("_STACK")) *__alloca(unsigned int __size);
#pragma aux __alloca =                  \
    "sub sp,ax"                         \
    __parm __nomemory [__ax]            \
    __value [__sp]                      \
    __modify __exact __nomemory [__sp]
#endif

#ifdef __C86__
void *__alloca(size_t);
#define alloca(s)           __alloca(s)
#endif

#endif
