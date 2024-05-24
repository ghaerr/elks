/* ELKS symbol table support */
#include <sys/cdefs.h>

/* symbol table format
 *  | byte type | word address | byte symbol length | symbol |
 *  type: (lower case means static)
 *      T, t    .text
 *      F, f    .fartext
 *      D, d    .data
 *      B, b    .bss
 *      0       end of symbol table
 */

#define next(sym)   \
    ((sym) + 1 + sizeof(unsigned short) + ((unsigned char __far *)sym)[SYMLEN] + 1)
#define TYPE        0
#define ADDR        1
#define SYMLEN      3
#define SYMBOL      4

unsigned char __far * noinstrument sym_read_exe_symbols(char *path);
unsigned char __far * noinstrument sym_read_symbols(char *path);
char * noinstrument sym_text_symbol(void *addr, int offset);
char * noinstrument sym_ftext_symbol(void *addr, int offset);
char * noinstrument sym_data_symbol(void *addr, int offset);
char * noinstrument sym_symbol(void *addr, int offset);
void * noinstrument sym_fn_start_address(void *addr);

extern struct minix_exec_hdr sym_hdr;
