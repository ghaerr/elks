/*
 * Watcom C startup and exit.
 *
 *  2 Jun 2024 Greg Haerr
 * 18 Jun 2024 Now rewrites argv/environ arrays for compact/large models
 */

#include <sys/cdefs.h>
#include <sys/rtinit.h>
#include <unistd.h>
#include <errno.h>

/* Watcom extern code refs are sym_, extern data refs are _sym */

extern void sys_exit(int status);
#pragma aux sys_exit =         \
    "xchg ax, bx"           \
    "xor ax, ax"            \
    "inc ax"                \
    "int 80h"               \
    __parm [ __ax ];

noreturn void _exit(int status)
{
    sys_exit(status);
}

noreturn void exit(int status)
{
    __FiniRtns();
    _exit(status);
}

/*
 * Force no register saves in main(), saves space.
 * This code is repeated in libc/include/sys/cdefs.h
 *
 * main(ac, av) called by AX, DX (small/medium) or AX, CX:BX (compact/large) model
 */
extern int main(int argc, char **argv);

/* global variables initialized at C startup */
int __argc;
char **__argv;
char *__program_filename;
char **environ;
unsigned int __stacklow;

static unsigned int _SP(void);
#pragma aux _SP = __value [__sp]

/* called by alloca() to check stack available */
unsigned int stackavail(void)
{
    return (_SP() - __stacklow);
}

#if defined(__SMALL__) || defined(__MEDIUM__)   /* near data models */
/* no argv/environ rewrite */
static noreturn void premain(void)
{
    __InitRtns();
    exit(main(__argc, __argv));
}
#else
/* rewrite argv and environ arrays in compact and large models, args in AX DX BX CX */
static noreturn void premain(char __near *newsp, char __near *oldsp, int bx, int n)
{
    char __far  * __far  *nap = (char __far  * __far  *)newsp;
    char __near * __near *oap = (char __near * __near *)oldsp;
    unsigned int v;
    __argv = (char **)newsp;
    n >>= 1;    /* length doubled so new argv array doesn't overwrite old one for 'ps' */
    do {
        if (v = (unsigned)*oap) {
            *nap = *oap;
        } else {
            *nap = 0;
        }
        ++nap;
        ++oap;
        n -= 2;
        if (n && !v)
            environ = nap;
    } while (n > 0);
    __InitRtns();
    exit(main(__argc, __argv));
}
#endif

/* DX contains program stack size at startup */
noreturn static void _crt0(void);
#if defined(__SMALL__) || defined(__MEDIUM__)   /* near data models */
#pragma aux _crt0 =         \
    "mov ax,sp"             \
    "sub ax,dx"             \
    "mov word ptr __stacklow, ax"   \
    "pop ax"                \
    "mov word ptr __argc, ax"       \
    "mov bx, sp"            \
    "mov word ptr __argv, bx"       \
    "next_env: cmp word ptr [bx], 1" \
    "inc bx"                \
    "inc bx"                \
    "jnc next_env"          \
    "mov word ptr environ, bx" \
    "mov bx, sp"            \
    "mov bx,word ptr [bx]"  \
    "mov word ptr __program_filename, bx"       \
    "push ax"               \
    "call premain";

#else

#pragma aux _crt0 =         \
    "mov ax,sp"             \
    "sub ax,dx"             \
    "mov word ptr __stacklow, ax"   \
    "pop ax"                \
    "mov word ptr __argc, ax"       \
    "mov bx, sp"            \
    "mov cx, ds"            \
    "mov word ptr __argv, bx"       \
    "mov word ptr __argv+2,cx"      \
    "mov word ptr __program_filename+2, cx"     \
    "mov word ptr environ+2, cx" \
    "next_env: cmp word ptr [bx], 1" \
    "inc bx"                \
    "inc bx"                \
    "jnc next_env"          \
    "mov word ptr environ, bx" \
    "next_env2: cmp word ptr [bx], 1" \
    "inc bx"                \
    "inc bx"                \
    "jnc next_env2"         \
    "sub bx, sp"            \
    "mov cx, bx"            \
    "shl cx,1"              \
    "mov dx, sp"            \
    "mov bx, sp"            \
    "mov bx,word ptr [bx]"  \
    "mov word ptr __program_filename, bx"       \
    "mov bx, cx"            \
    "push ax"               \
    "sub sp, bx"            \
    "mov ax, sp"            \
    "call premain";
#endif

#if defined(__SMALL__) || defined(__COMPACT__)  /* near code models */
/* jumped from _start for prevention of zero near function address */
#pragma aux _start_crt0 "*"
noreturn void _start_crt0(void) { _crt0(); }

#else

/* actual program entry point for far code (medium and large) models */
#pragma aux _start "*"
noreturn void _start(void) { _crt0(); }
#endif
