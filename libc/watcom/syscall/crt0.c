/*
 * Watcom C startup and exit.
 *
 * 2 Jun 2024 Greg Haerr
 */

#include <sys/cdefs.h>
#include <unistd.h>
#include <errno.h>

/* external references created by Watcom C compilation - unused */
int cstart_;            /* with declaration of main() */
int _argc;              /* with declaration of main() */
int _8087;              /* when floating point seen */

/* FIXME near/far declaration of environ works only in compact/large models */
char __wcnear * __wcfar *environ;

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
 * For now, argv array is left unmodified requiring special main declaration.
 * This code is repeated in libc/include/sys/cdefs.h
 */
#pragma aux main "*" modify [ bx cx dx si di ]
int main(int argc, char __wcnear * __wcfar *argv);

#if defined(__SMALL__) || defined(__MEDIUM__)
noreturn static void _crt0(void);
#pragma aux _crt0 =         \
    "pop ax"                \
    "mov dx, sp"            \
    "mov bx, sp"            \
    "next_env: cmp word ptr [bx], 1" \
    "inc bx"                \
    "inc bx"                \
    "jnc next_env"          \
    "mov word ptr environ, bx" \
    "call main"             \
    "call exit";
#else
#pragma aux _crt0 =         \
    "pop ax"                \
    "mov bx, sp"            \
    "mov dx, bx"            \
    "mov cx, ds"            \
    "next_env: cmp word ptr [bx], 1" \
    "inc bx"                \
    "inc bx"                \
    "jnc next_env"          \
    "mov word ptr environ, bx" \
    "mov word ptr environ+2, cx" \
    "mov bx, dx"            \
    "call main"             \
    "call exit";
#endif

/* actual program entry point */
noreturn void _start(void) { _crt0(); }
