/* ELKS disassembler header file */

#define noinstrument    __attribute__((no_instrument_function))

/* to be defined by caller of disasm() */
char * noinstrument getsymbol(int seg, int offset);

/* disasm.c */
int disasm(int cs, int ip, int (*nextbyte)(int, int));

/* printreg.S */
int noinstrument getcs(void);
