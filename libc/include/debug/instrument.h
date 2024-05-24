/* ELKS stack trace and instrumentation functions library */
/* Jan 2023 Greg Haerr */
#include <sys/cdefs.h>

/* calc_push_count returns */
#define BP_PUSHED   0x0100
#define DI_PUSHED   0x0200
#define SI_PUSHED   0x0400
#define COUNT_MASK  0x0007

/* instrumentation functions called when -finstrument-functions-simple set */
void noinstrument __cyg_profile_func_enter_simple(void);
void noinstrument __cyg_profile_func_exit_simple(void);
unsigned long noinstrument _get_micro_count(void);

/* stacktrace.c */
void noinstrument _print_stack(int arg1);

/* printreg.S */
void noinstrument _print_regs(void);
void noinstrument _print_segs(void);

/* readprologue.c */
int * noinstrument _get_fn_start_address(int *addr);
int noinstrument _get_push_count(int *fnstart);

/* rdtsc.S */
unsigned long long noinstrument _get_rdtsc(void);
