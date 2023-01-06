#ifndef __SETJMP_H
#define __SETJMP_H

#include <features.h>

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
void _longjmp(jmp_buf env, int rv);

/* LATER: Seems GNU beat me to it, must be OK then :-)
 *        Humm, what's this about setjmp being a macro !?
 *        Ok, use the BSD names as normal use the ANSI as macros
 */

#define setjmp(a_env)           _setjmp(a_env)
#define longjmp(a_env, a_rv)	_longjmp(a_env, a_rv)
#endif
