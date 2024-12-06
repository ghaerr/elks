/*
 * Open Watcom C stack checking helper routines
 *
 * 4 Dec 2024 Greg Haerr
 */

#include <sys/cdefs.h>
#include <alloca.h>
#include <unistd.h>

extern unsigned int __stacklow;

#define errmsg(str)     write(STDERR_FILENO, str, sizeof(str) - 1)

static unsigned int __SP(void);
#pragma aux __SP = __value [__sp]

static void stack_alloca_warning(void)
{
    errmsg("ALLOCA FAIL, INCREASE STACK\n");
}

/* 
 * Return true if stack can be extended by size bytes,
 * called by alloca() to check stack available.
 */
#pragma aux __stackavail "*"
int __stackavail(unsigned int size)
{
    unsigned int remaining = __SP() - __stacklow;

    if ((int)remaining >= 0 && remaining >= size)
        return 1;
    stack_alloca_warning();
    return 0;
}

#if LATER
static void stack_overflow(void)
{
    errmsg("STACK OVERFLOW\n");
}

static void stack_overlimit(void)
{
    errmsg("STACK OVER LIMIT\n");
}
#endif

/* 
 * Check if size bytes can be allocated from stack,
 * called from function prologue when -fstack-check set.
 */
#pragma aux __STK "*"
void __STK(unsigned int size)
{
    unsigned int remaining = __SP() - __stacklow;
    unsigned int curbreak;

    if ((int)remaining >= 0 && remaining >= size)
        return;
    curbreak = (unsigned int)sbrk(0);   /* NOTE syscall here will cause SIGSEGV sent */
#if LATER
    if (__SP() < curbreak)
        stack_overflow();
    else
        stack_overlimit();
#endif
}
