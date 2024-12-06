/*
 * IA16 stack checking helper routines
 *
 * 5 Dec 2024 Greg Haerr
 */

#include <sys/cdefs.h>
#include <alloca.h>
#include <unistd.h>
#include <stdio.h>

extern unsigned int __stacklow;

#define __SP()          ((unsigned int)__builtin_frame_address(0))  /* NOTE not exact */
#define errmsg(str)     write(STDERR_FILENO, str, sizeof(str) - 1)

/* 
 * Return true if stack can be extended by size bytes,
 * called by alloca() to check stack available.
 */
int __stackavail(unsigned int size)
{
    unsigned int remaining = __SP() - __stacklow;

    if ((int)remaining >= 0 && remaining >= size)
        return 1;
    errmsg("ALLOCA FAIL, INCREASE STACK\n");
    return 0;
}

#if 0
/* 
 * Check if size bytes can be allocated from stack,
 * called from function prologue when -fstack-check set.
 */
void __STK(unsigned int size)
{
    unsigned int remaining = __SP() - __stacklow;
    unsigned int curbreak;

    if ((int)remaining >= 0 && remaining >= size)
        return;
    curbreak = (unsigned int)sbrk(0);   /* NOTE syscall here will cause SIGSEGV sent */
#if LATER
    if (__SP() < curbreak)
        errmsg("STACK OVERFLOW\n");
    else
        errmsg("STACK OVER LIMIT\n");
#endif
}
#endif
