/* ELKS stack trace and instrumentation functions library */

#define noinstrument    __attribute__((no_instrument_function))

/* stacktrace.c */
void noinstrument print_stack(int arg1);

/* instrumentation functions called when -finstrument-functions set */
void noinstrument __cyg_profile_func_enter(void *, void *);
void noinstrument __cyg_profile_func_exit(void *, void *);
void print_times(void);

/* printreg.S */
void noinstrument print_regs(void);
void noinstrument print_sreg(void);
int noinstrument getcsbyte(char *addr);
int noinstrument getcs(void);
unsigned long long noinstrument rdtsc(void);

/* disasm.c */
void * noinstrument disasm(unsigned short cs, void *ip);

/* ulltostr.c */
char * noinstrument lltostr(long long val, int radix);
char * noinstrument ulltostr(unsigned long long val, int radix);
char * noinstrument lltohexstr(unsigned long long val);
