/*
 * Watcom C startup and exit.
 *
 *  2 Jun 2024 Greg Haerr
 * 18 Jun 2024 Now rewrites argv/environ arrays for compact/large models
 */

#include <sys/cdefs.h>
#include <unistd.h>
#include <errno.h>

/* external references created by Watcom C compilation - unused */
int _argc;              /* with declaration of main() */
int _8087;              /* when floating point seen */

noreturn void _exit(int status);
#pragma aux _exit =         \
    "xchg ax, bx"           \
    "xor ax, ax"            \
    "inc ax"                \
    "int 80h"               \
    __parm [ __ax ];

noreturn void exit(int status)
{
    _exit(status);
}

/*
 * Force no register saves in main(), saves space.
 * This code is repeated in libc/include/sys/cdefs.h
 *
 * main(ac, av) called by AX, DX (small/medium) or AX, CX:BX (compact/large) model
 */
#pragma aux main "*" modify [ bx cx dx si di ]
extern int main(int argc, char **argv);

/* global variables initialized at C startup */
int __argc;
char **__argv;
char *__program_filename;
char **environ;

/* cstart_ is an external reference created by Watcom C compilation, used here */
#pragma aux cstart_ "_*" modify [ bx cx dx si di ]

#if defined(__SMALL__) || defined(__MEDIUM__)
noreturn void cstart_(void)
{
    exit(main(__argc, __argv));
}
#else
/* rewrite argv and environ arrays in compact and large models */
noreturn void cstart_(char __near *newsp, char __near *oldsp, int bx, int n)
{
    char __far * __far *nap = newsp;
    char __near * __near *oap = oldsp;
    unsigned int v;
    __argv = newsp;
    do {
        if (v = *oap) {
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
    exit(main(__argc, __argv));
}
#endif

noreturn static void _crt0(void);
#if defined(__SMALL__) || defined(__MEDIUM__)
#pragma aux _crt0 =         \
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
    "call cstart_";
#else
#pragma aux _crt0 =         \
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
    "mov dx, sp"            \
    "mov bx, sp"            \
    "mov bx,word ptr [bx]"  \
    "mov word ptr __program_filename, bx"       \
    "mov bx, cx"            \
    "sub sp, bx"            \
    "mov ax, sp"            \
    "call cstart_";
#endif

/* actual program entry point */
noreturn void _start(void) { _crt0(); }
