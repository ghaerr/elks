/* ELKS disassembler header file */

#define noinstrument    __attribute__((no_instrument_function))

/* to be defined by caller of disasm() */
char * noinstrument getsymbol(int seg, int offset);

/* disasm.c */
void * noinstrument disasm(int cs, void *ip);

/* printreg.S */
int noinstrument getcs(void);
