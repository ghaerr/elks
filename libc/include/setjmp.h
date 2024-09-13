#ifndef __SETJMP_H
#define __SETJMP_H

#include <features.h>

#ifdef __GNUC__
/* 
 * I know most systems use an array of ints here, but I prefer this   - RDB
 */

typedef struct
{
   unsigned int pc;
#if defined __MEDIUM__ || defined __LARGE__ || defined __HUGE__
   unsigned int cs;
#endif
   unsigned int sp;
   unsigned int bp;
   unsigned int si;
   unsigned int di;
   unsigned int es;
} jmp_buf[1];

int _setjmp(jmp_buf env);
noreturn void _longjmp(jmp_buf env, int rv);

#define setjmp(a_env)           _setjmp(a_env)
#define longjmp(a_env, a_rv)	_longjmp(a_env, a_rv)
#endif

#ifdef __WATCOMC__
#include <watcom/setjmp.h>
#endif

#endif
