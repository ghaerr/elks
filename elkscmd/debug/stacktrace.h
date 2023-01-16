/* ELKS stack trace and instrumentation functions library */

#define noinstrument    __attribute__((no_instrument_function))

/* calc_push_count returns */
#define BP_PUSHED   0x0100
#define DI_PUSHED   0x0200
#define SI_PUSHED   0x0400
#define COUNT_MASK  0x0007

/* stacktrace.c */
void noinstrument print_stack(int arg1);
int noinstrument calc_push_count(int *addr);

/* instrumentation functions called when -finstrument-functions-simple set */
void noinstrument __cyg_profile_func_enter_simple(void);
void noinstrument __cyg_profile_func_exit_simple(void);
unsigned long noinstrument get_micro_count(void);
void noinstrument print_times(void);

/* printreg.S */
void noinstrument print_regs(void);
void noinstrument print_sreg(void);
int noinstrument getcsbyte(char *addr);
unsigned long long noinstrument rdtsc(void);
