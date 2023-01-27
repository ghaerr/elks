/* ELKS disassembler header file */

#define noinstrument    __attribute__((no_instrument_function))

/* to be defined by caller of disasm() */
char * noinstrument getsymbol(int seg, int offset);
char * noinstrument getsegsymbol(int seg);

/* disasm.c */
int disasm(int cs, int ip, int (*nextbyte)(int, int), int ds);
extern int f_asmout;    /* =1 for asm output (no addresses or hex values) */
extern int f_outcol;    /* output column number (if !f_asmout) */
