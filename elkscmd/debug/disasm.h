/* ELKS disassembler header file */

#if defined(__ia16__) || defined(__WATCOMC__)
#include <sys/cdefs.h>
#else
#define noinstrument
#endif

/* to be defined by caller of disasm() */
char * noinstrument getsymbol(int seg, int offset);
char * noinstrument getsegsymbol(int seg);

/* disasm.c */
int disasm(int cs, int ip, int (*nextbyte)(int, int), int ds);
extern int f_asmout;    /* =1 for asm output (no addresses or hex values) */
extern int f_outcol;    /* output column number (if !f_asmout) */
