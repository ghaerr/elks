#ifndef __STDARG_H
#define __STDARG_H
/*
 * stdarg.h for C86
 *
 * 29 Nov 2024 Greg Haerr
 */

typedef char *      __va_list;
typedef __va_list   va_list;

#define va_start(vaptr,parm)    \
                (vaptr=(char *)&parm + \
                ((sizeof(parm)+sizeof(int)-1)&~(sizeof(int)-1)),(void)0)

#define va_arg(vaptr,__type)   \
                (vaptr += ((sizeof(__type)+sizeof(int)-1)&~(sizeof(int)-1)), \
                (*(__type *)(vaptr-((sizeof(__type)+sizeof(int)-1)&~(sizeof(int)-1)))))

#define va_end(vaptr)          (vaptr = 0,(void)0)

#endif
