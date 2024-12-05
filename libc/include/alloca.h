#ifndef __ALLOCA_H
#define __ALLOCA_H

// void *alloca(size_t);

int __stackavail(unsigned int size);
#define __ALLOCA_ALIGN(s)   (((s)+(sizeof(int)-1))&~(sizeof(int)-1))

#define alloca(s)           (__stackavail(__ALLOCA_ALIGN(s))?   \
                                 __alloca(__ALLOCA_ALIGN(s)): (void *)0)

#ifdef __WATCOMC__
#pragma aux __stackavail "*" __modify __nomemory

extern void __based(__segname("_STACK")) *__alloca(unsigned int __size);
#pragma aux __alloca =                  \
    "sub sp,ax"                         \
    __parm __nomemory [__ax]            \
    __value [__sp]                      \
    __modify __exact __nomemory [__sp]

#endif


#endif
